#include "sys.h"
#include <iostream>
#include <atomic>
#include "utils/FuzzyBool.h"
#include "utils/macros.h"
#include "ThreadPermuter.h"

class FuzzyLock;

class FuzzyTracker
{
 private:
  std::mutex m_mutex;
  FuzzyLock* m_first;

 private:
  void polute(FuzzyLock* ptr);

 public:
  FuzzyTracker() : m_first(nullptr) { }

  void lock(FuzzyLock* ptr)
  {
    DoutEntering(dc::notice|flush_cf, "lock([" << ptr << "])");
    std::lock_guard<std::mutex> lk(m_mutex);
    if (AI_UNLIKELY(m_first != nullptr))
      polute(m_first);
    Dout(dc::notice|flush_cf, "Setting m_first to " << ptr);
    m_first = ptr;
  }

  void unlock(FuzzyLock* ptr)
  {
    DoutEntering(dc::notice|flush_cf, "unlock([" << ptr << "])");
    std::lock_guard<std::mutex> lk(m_mutex);
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
  std::atomic_bool m_poluted;

 public:
  int const thread;

  FuzzyLock(int thr, FuzzyTracker& tracker) : m_tracker(tracker), m_poluted(false), thread(thr)
  {
    DoutEntering(dc::notice|flush_cf, "FuzzyLock() [" << this << "])");
    m_tracker.lock(this);
  }

  ~FuzzyLock()
  {
    DoutEntering(dc::notice|flush_cf, "~FuzzyLock() [" << this << "])");
    m_tracker.unlock(this);
  }

  void polute()
  {
    DoutEntering(dc::notice|flush_cf, "polute() [" << this << "])");
    m_poluted = true;
  }

  bool is_poluted() const
  {
    return m_poluted;
  }
};

void FuzzyTracker::polute(FuzzyLock* ptr)
{
  Dout(dc::notice|flush_cf, "Calling ptr->polute()");
  ptr->polute();
}

class B : public FuzzyTracker
{
 private:
  std::mutex m_started_mutex;
  bool m_started;
  utils::FuzzyBool m_empty;

 public:
  B(bool started, bool empty) : m_started(started), m_empty(empty) { }
  friend std::ostream& operator<<(std::ostream& os, B const& b);

  utils::FuzzyBool is_empty(FuzzyLock& fl) const
  {
    // The FuzzyLock is unused here - the parameter is only there to force the user to create a FuzzyLock object.
    Dout(dc::notice|flush_cf, "thread " << fl.thread << ": Testing (m_empty = " << m_empty << ")");
    return m_empty;
  }

  void set_empty(utils::FuzzyBool empty)
  {
    m_empty = empty;
  }

  void start_if(utils::FuzzyBool condition, FuzzyLock& fl)
  {
    Dout(dc::notice|flush_cf, "thread " << fl.thread << ": Calling start(" << condition << ")");
    ASSERT(!condition.unlikely());
    std::lock_guard<std::mutex> lk(m_started_mutex);
    if (condition.never())
      return;
    if (condition.likely())
    {
    }
    Dout(dc::notice|flush_cf, "Setting m_started to true.");
    m_started = true;
  }

  void stop_if(utils::FuzzyBool condition, FuzzyLock& fl)
  {
    Dout(dc::notice|flush_cf, "thread " << fl.thread << ": Calling stop_if(" << condition << ")");
    ASSERT(!condition.unlikely());
    std::lock_guard<std::mutex> lk(m_started_mutex);
    if (condition.never())
      return;
    if (condition.likely())
    {
      if (fl.is_poluted())
      {
        Dout(dc::notice|flush_cf, "Ignoring call to stop_if() because condition is WasTrue and FuzzyLock is poluted.");
        return;
      }
    }
    Dout(dc::notice|flush_cf, "Setting m_started to false.");
    m_started = false;
  }

  std::string m_permutation;

  void on_permutation_begin();
  void on_permutation_end();
};

void B::on_permutation_begin()
{
  DoutEntering(dc::notice|flush_cf, "on_permutation_begin()");
  m_permutation = "";
  Dout(dc::notice|flush_cf, "Before: " << *this);
}

void B::on_permutation_end()
{
  DoutEntering(dc::notice|flush_cf, "on_permutation_end()");
  Dout(dc::notice|flush_cf, "Permutation: " << m_permutation << "; Result: " << *this);
}

void thread0(B& b)
{
  b.m_permutation += '0';
  Dout(dc::notice|flush_cf, "thread 0: calling set_empty(fuzzy::WasTrue).");
  b.set_empty(fuzzy::WasTrue);
  TPY;

  b.m_permutation += '0';
  Dout(dc::notice|flush_cf, "thread 0: creating FuzzyLock.");
  FuzzyLock a(0, b);
  TPY;

  b.m_permutation += '0';
  Dout(dc::notice|flush_cf, "thread 0: creating FuzzyBool.");
  utils::FuzzyBool empty = b.is_empty(a);
  TPY;

  Dout(dc::notice|flush_cf, "thread 0: testing empty.");
  if (empty.likely())               // If empty, stop.
  {
    b.m_permutation += '0';
    b.stop_if(empty, a);
  }
}

#if 0
void thread0(B& b)
{
  b.set_empty(fuzzy::WasTrue);
  FuzzyLock empty(b, b.is_empty());
  b.stop_if(empty); // Stop reading empty buffer.
}

void thread1(B& b)
{
  b.set_empty(fuzzy::WasFalse);
  FuzzyLock not_empty(b, !b.is_empty());
  b.start_if(not_empty); // Start reading from buffer.
}
#endif

void thread1(B& b)
{
  b.m_permutation += '1';
  Dout(dc::notice|flush_cf, "thread 1: calling set_empty(fuzzy::WasFalse).");
  b.set_empty(fuzzy::WasFalse);
  TPY;

  b.m_permutation += '1';
  Dout(dc::notice|flush_cf, "thread 1: creating FuzzyLock.");
  FuzzyLock a(1, b);
  TPY;

  b.m_permutation += '1';
  Dout(dc::notice|flush_cf, "thread 1: creating FuzzyBool.");
  utils::FuzzyBool empty = b.is_empty(a);
  TPY;

  b.m_permutation += '1';
  Dout(dc::notice|flush_cf, "thread 1: testing empty.");
  if (empty.unlikely())              // If not empty, start.
  {
    b.m_permutation += '1';
    b.start_if(!empty, a);
  }
}

int main()
{
  Debug(NAMESPACE_DEBUG::init());

  for (int b1 = 1; b1 < 2; ++b1)
    for (int b2 = 0; b2 < 1; ++b2)
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
          [&]{ b.on_permutation_end(); }
      );

      tp.run("0001111");

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
