#pragma once

#include <functional>
#include <thread>
#include <condition_variable>

namespace thread_permuter {

class Thread
{
 public:
  Thread(std::function<void()> test);

  void start();                         // Start the thread and prepare calling step().
  void run();                           // Entry point of m_thread.
  bool step();                          // Wake up the thread and let it run till the next check point (or finish).
                                        // Returns true when m_test() returned.
  void pause();                         // Pause the thread and wake up the main thread again.
  void stop();                          // Called when all permutation have been run.

 private:
  std::function<void()> m_test;         // Thread entry point. The first time step() is called
                                        // after start(), this function will be called.
  std::thread m_thread;                 // The actual thread.
  bool m_permutation_finished;          // True after returning from m_test.
  bool m_last_permutation;              // True after all permutation have been run.

  std::condition_variable m_paused_condition;
  std::mutex m_paused_mutex;
  bool m_paused;                        // True when the thread is waiting.
};

} // namespace thread_permuter
