#pragma once

#include <functional>
#include <thread>
#include <condition_variable>

namespace thread_permuter {

enum state_type
{
  yielding,     // Just yielding.
  blocking,     // This thread should not be run anymore until at least one other thread did run.
  finished      // Returned from m_test().
};

class Thread
{
 public:
  Thread(std::function<void()> test);

  void start();                         // Start the thread and prepare calling step().
  void run();                           // Entry point of m_thread.
  state_type step();                    // Wake up the thread and let it run till the next check point (or finish).
                                        // Returns true when m_test() returned.
  void pause(state_type state);         // Pause the thread and wake up the main thread again.
  void stop();                          // Called when all permutation have been run.

 private:
  std::function<void()> m_test;         // Thread entry point. The first time step() is called
                                        // after start(), this function will be called.
  std::thread m_thread;                 // The actual thread.
  state_type m_state;
  bool m_last_permutation;              // True after all permutation have been run.

  std::condition_variable m_paused_condition;
  std::mutex m_paused_mutex;
  bool m_paused;                        // True when the thread is waiting.

  static thread_local Thread* tl_self;  // A thread_local pointer to self.

 public:
  static void yield() { tl_self->pause(yielding); }
  static void blocked() { tl_self->pause(blocking); }
};

} // namespace thread_permuter

// Use this to make the thread yield and either continue with a different thread or with the same thread again.
#define TPY do { thread_permuter::Thread::yield(); } while(0)
// Use this to make the thread yield and force the run of another thread before running this thread again.
#define TPB do { thread_permuter::Thread::blocked(); } while(0)
