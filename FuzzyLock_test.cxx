#include "sys.h"
#include <iostream>
#include <atomic>
#include "utils/FuzzyBool.h"
#include "utils/macros.h"
#include "ThreadPermuter.h"

using namespace thread_permuter;

class FuzzyLock;

class FuzzyTracker
{
 private:
  Mutex m_mutex;
  FuzzyLock* m_first;

 private:
  void pollute(FuzzyLock* ptr);

 public:
  FuzzyTracker() : m_first(nullptr) { }

  void lock(FuzzyLock* ptr)
  {
    DoutEntering(dc::notice|flush_cf, "lock([" << ptr << "])");
    std::lock_guard<Mutex> lk(m_mutex);
    if (AI_UNLIKELY(m_first != nullptr))
      pollute(m_first);
    Dout(dc::notice|flush_cf, "Setting m_first to " << ptr);
    m_first = ptr;
  }

  void unlock(FuzzyLock* ptr)
  {
    DoutEntering(dc::notice|flush_cf, "unlock([" << ptr << "])");
    std::lock_guard<Mutex> lk(m_mutex);
    if (AI_LIKELY(m_first == ptr))
    {
      Dout(dc::notice|flush_cf, "Setting m_first to nullptr");
      m_first = nullptr;
    }
  }
};

class FuzzyLock
{
 private:
  FuzzyTracker& m_tracker;
  std::atomic_bool m_polluted;

 public:
  FuzzyLock(FuzzyTracker& tracker) : m_tracker(tracker), m_polluted(false)
  {
    DoutEntering(dc::notice|flush_cf, "thread " << Thread::name() << ": FuzzyLock() [" << this << "])");
    m_tracker.lock(this);
  }

  ~FuzzyLock()
  {
    DoutEntering(dc::notice|flush_cf, "thread " << Thread::name() << ": ~FuzzyLock() [" << this << "])");
    m_tracker.unlock(this);
  }

  void pollute()
  {
    DoutEntering(dc::notice|flush_cf, "thread " << Thread::name() << ": pollute() [" << this << "])");
    m_polluted = true;
  }

  bool is_polluted() const
  {
    return m_polluted;
  }
};

void FuzzyTracker::pollute(FuzzyLock* ptr)
{
  Dout(dc::notice|flush_cf, "thread " << Thread::name() << ": Calling ptr->pollute()");
  ptr->pollute();
}

class B : public FuzzyTracker
{
 private:
  Mutex m_started_mutex;
  bool m_started;
  bool m_started_expected;
  utils::FuzzyBool m_empty;

 public:
  B(bool started, bool empty) : m_started(started), m_empty(empty) { }
  friend std::ostream& operator<<(std::ostream& os, B const& b);

  utils::FuzzyBool is_empty(FuzzyLock& UNUSED_ARG(fl)) const
  {
    // The FuzzyLock is unused here - the parameter is only there to force the user to create a FuzzyLock object.
    Dout(dc::notice|flush_cf, "thread " << Thread::name() << ": Testing (m_empty = " << m_empty << ")");
    return m_empty;
  }

  void set_empty(utils::FuzzyBool empty)
  {
    Dout(dc::notice|flush_cf, "thread " << Thread::name() << ": calling set_empty(" << empty << ").");
    m_empty = empty;
  }

  void set_expected(bool started)
  {
    m_started_expected = started;
  }

  void start_if(utils::FuzzyBool condition, FuzzyLock& fl)
  {
    Dout(dc::notice|flush_cf, "thread " << Thread::name() << ": Calling start(" << condition << ")");
    ASSERT(!condition.unlikely());
    if (condition.never())
      return;
    std::lock_guard<Mutex> lk(m_started_mutex);
    if (!condition.always() && condition.likely())
    {
      if (fl.is_polluted())
      {
        Dout(dc::notice|flush_cf, "Ignoring call to start_if() because condition is WasTrue and FuzzyLock is polluted.");
        return;
      }
    }
    Dout(dc::notice|flush_cf, "Setting m_started to true.");
    m_started = true;
  }

  void stop_if(utils::FuzzyBool condition, FuzzyLock& fl)
  {
    Dout(dc::notice|flush_cf, "thread " << Thread::name() << ": Calling stop_if(" << condition << ")");
    ASSERT(!condition.unlikely());
    if (condition.never())
      return;
    std::lock_guard<Mutex> lk(m_started_mutex);
    if (!condition.always() && condition.likely())      // condition == WasTrue
    {
      if (fl.is_polluted())
      {
        Dout(dc::notice|flush_cf, "Ignoring call to stop_if() because condition is WasTrue and FuzzyLock is polluted.");
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

  FuzzyLock fl(b);                              // Lock
  TPY;

  utils::FuzzyBool empty = b.is_empty(fl);      // Acquire

  if (empty.likely())                           // If empty, stop.
  {
    TPY;
    b.stop_if(empty, fl);
  }
}

void thread1(B& b)
{
  b.set_empty(fuzzy::WasFalse);
  b.set_expected(true);
  TPY;

  FuzzyLock fl(b);
  TPY;

  utils::FuzzyBool empty = b.is_empty(fl);

  if (empty.unlikely())              // If not empty, start.
  {
    TPY;
    b.start_if(!empty, fl);
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
  if (b.m_empty.always())
    os << " (empty)";
  else if (b.m_empty.likely())
    os << " (likely empty)";
  else if (b.m_empty.never())
    os << " (not empty)";
  else
    os << " (unlikely empty)";
  if (b.m_empty.unlikely() && !b.m_started)
    os << " [ERROR]";
  if (b.m_empty.likely() && b.m_started)
    os << " [INEFFICIENT]";
  return os;
}
