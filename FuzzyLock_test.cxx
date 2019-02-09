#include "sys.h"
#include <iostream>
#include <atomic>
#include "utils/FuzzyBool.h"
#include "utils/macros.h"
#include "ThreadPermuter.h"

using namespace thread_permuter;

class B
{
 private:
  Mutex m_started_mutex;
  bool m_started_init;
  bool m_empty_init;
  bool m_started;
  bool m_started_expected;
  utils::FuzzyBool m_empty;

 public:
  B(bool started, bool empty) : m_started_init(started), m_empty_init(empty) { }
  friend std::ostream& operator<<(std::ostream& os, B const& b);

  utils::FuzzyBool is_empty() const
  {
    Dout(dc::notice|flush_cf, "thread " << Thread::name() << ": Testing (m_empty = " << m_empty << ")");
    return m_empty;
  }

  void set_empty(utils::FuzzyBoolPOD empty)
  {
    Dout(dc::notice|flush_cf, "thread " << Thread::name() << ": calling set_empty(" << empty << ").");
    m_empty = empty;
  }

  void set_expected(bool started)
  {
    m_started_expected = started;
  }

  void start_if(utils::FuzzyBool condition)
  {
    Dout(dc::notice|flush_cf, "thread " << Thread::name() << ": Calling start_if(" << condition << ")");
    // Only call this function when the condition is momentary true.
    ASSERT(condition.is_momentary_true());
    std::lock_guard<Mutex> lk(m_started_mutex);
    if (condition.is_transitory_true())
    {
      Dout(dc::notice|flush_cf, "Calling is_empty()");
      if (is_empty().is_momentary_true())
      {
        Dout(dc::notice|flush_cf, "Ignoring call to start_if() because condition is WasTrue and is_empty() returns true.");
        return;
      }
    }
    Dout(dc::notice|flush_cf, "Setting m_started to true.");
    m_started = true;
  }

  void stop_if(utils::FuzzyBool const& condition)
  {
    Dout(dc::notice|flush_cf, "thread " << Thread::name() << ": Calling stop_if(" << condition << ")");
    // Only call this function when the condition is momentary true.
    ASSERT(condition.is_momentary_true());
    std::lock_guard<Mutex> lk(m_started_mutex);
    if (condition.is_transitory_true())
    {
      Dout(dc::notice|flush_cf, "Calling is_empty()");
      if (is_empty().is_momentary_false())
      {
        Dout(dc::notice|flush_cf, "Ignoring call to stop_if() because condition is WasTrue and is_empty() returns false.");
        return;
      }
    }
    Dout(dc::notice|flush_cf, "Setting m_started to false.");
    m_started = false;
  }

  void on_permutation_begin();
  void on_permutation_end(std::string const& permutation);
};

void B::on_permutation_begin()
{
  DoutEntering(dc::notice|flush_cf, "on_permutation_begin()");
  Dout(dc::notice|flush_cf, "Before: " << *this);
  m_started = m_started_init;
  m_empty = m_empty_init ? fuzzy::True : fuzzy::False;
}

void B::on_permutation_end(std::string const& permutation)
{
  DoutEntering(dc::notice|flush_cf, "on_permutation_end(\"" << permutation << "\")");
  Dout(dc::notice|flush_cf, "Result: " << *this);
  ASSERT(m_started == m_started_expected);
}

void thread0(B& b)
{
  b.set_empty(fuzzy::WasTrue);                  // Release
  b.set_expected(false);
  TPY;

  utils::FuzzyBool empty = b.is_empty();        // Acquire
  TPY;

  if (empty.is_momentary_true())                // If empty, stop.
  {
    TPY;
    b.stop_if(empty);
  }
}

void thread1(B& b)
{
  b.set_empty(fuzzy::WasFalse);
  b.set_expected(true);
  TPY;

  utils::FuzzyBool empty = b.is_empty();
  TPY;

  if (empty.is_momentary_false())               // If not empty, start.
  {
    TPY;
    b.start_if(!empty);
  }
}

int main()
{
  Debug(NAMESPACE_DEBUG::init());

  for (int b1 = 0; b1 < 2; ++b1)
    for (int b2 = 0; b2 < 2; ++b2)
    {
      B b(b1, b2);

      ThreadPermuter::tests_type tests =
      {
        [&b]{ thread0(b); },
        [&b]{ thread1(b); }
      };

      ThreadPermuter tp(
          [&]{ b.on_permutation_begin(); },
          tests,
          [&](std::string const& permutation){ b.on_permutation_end(permutation); }
      );

      tp.run();

      Dout(dc::notice|flush_cf, "After: " << b);
    }
}

std::ostream& operator<<(std::ostream& os, B const& b)
{
  if (b.m_started)
    os << "Started";
  else
    os << "Stopped";
  if (b.m_empty.is_true())
    os << " (empty)";
  else if (b.m_empty.is_transitory_true())
    os << " (transitory empty)";
  else if (b.m_empty.is_false())
    os << " (not empty)";
  else
    os << " (transitory not empty)";
  if (b.m_empty.is_momentary_false() && !b.m_started)
    os << " [ERROR]";
  if (b.m_empty.is_momentary_true() && b.m_started)
    os << " [INEFFICIENT]";
  return os;
}
