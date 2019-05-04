#pragma once

#include "debug.h"
#include <functional>
#include <thread>
#include <condition_variable>

#if defined(CWDEBUG) && !defined(DOXYGEN)
NAMESPACE_DEBUG_CHANNELS_START
extern channel_ct permutation;
NAMESPACE_DEBUG_CHANNELS_END
#endif

class PermutationFailure : public std::runtime_error
{
 public:
  PermutationFailure(char const* msg) : std::runtime_error(msg) { }
};

namespace thread_permuter {

enum state_type
{
  yielding,     // Just yielding.
  blocking,     // This thread should not be run anymore until at least one other thread did run.
  failed,       // This thread encountered an error condition and threw an exception.
  finished      // Returned from m_test().
};

class Thread
{
 public:
  Thread(std::function<void()> test);

  void start(char thread_name, bool debug_off); // Start the thread and prepare calling step().
  void run(bool debug_off);             // Entry point of m_thread.
  state_type step(bool& debug_on);      // Wake up the thread and let it run till the next check point (or finish).
                                        // Returns true when m_test() returned.
  void pause(state_type state);         // Pause the thread and wake up the main thread again.
  void stop();                          // Called when all permutation have been run.

  char get_name() const { return m_thread_name; }
  std::string const& what() const { return m_what; }

 private:
  std::function<void()> m_test;         // Thread entry point. The first time step() is called
                                        // after start(), this function will be called.
  std::thread m_thread;                 // The actual thread.
  state_type m_state;
  bool m_last_permutation;              // True after all permutation have been run.

  std::condition_variable m_paused_condition;
  std::mutex m_paused_mutex;
  bool m_paused;                        // True when the thread is waiting.
  bool m_debug_on;                      // Set to true when debug output must be turned on in this thread.
  std::string m_what;                   // Error of last exception thrown.

  char m_thread_name;                   // Used for debugging output; set by start().

  static thread_local Thread* tl_self;  // A thread_local pointer to self.

 public:
  static void yield() { tl_self->pause(yielding); }
  static void blocked() { tl_self->pause(blocking); }
  static void fail(char const* what) { tl_self->m_what = what; tl_self->pause(failed); }
  static char name() { return tl_self->get_name(); }
};

// Use this instead of std::mutex.
class Mutex
{
 private:
  std::mutex m_mutex;

 public:
  void lock()
  {
    DoutEntering(dc::permutation|flush_cf|continued_cf, "lock()... ");
    while (!m_mutex.try_lock())
    {
      Dout(dc::permutation|flush_cf, "Blocked on mutex.");
      Thread::blocked();
    }
    Dout(dc::finish|flush_cf, "locked");
  }

  bool try_lock()
  {
    DoutEntering(dc::permutation|flush_cf|continued_cf, "try_lock()... ");
    bool locked = m_mutex.try_lock();
    Dout(dc::finish|flush_cf, (locked ? "locked" : "failed"));
    return locked;
  }

  void unlock()
  {
    Dout(dc::permutation|flush_cf, "unlock()");
    m_mutex.unlock();
  }

  auto native_handle()
  {
    return m_mutex.native_handle();
  }
};

} // namespace thread_permuter

// Use this to make the thread yield and either continue with a different thread or with the same thread again.
#define TPY do { thread_permuter::Thread::yield(); } while(0)
// Use this to make the thread yield and force the run of another thread before running this thread again.
#define TPB do { thread_permuter::Thread::blocked(); } while(0)
