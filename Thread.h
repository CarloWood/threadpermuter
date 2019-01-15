#pragma once

#include <functional>

namespace thread_permuter {

class Thread
{
 public:
  Thread(std::function<void()> test);

  void start();                         // Start the thread and prepare calling step().
  bool step();                          // Run the thread to the next check point (or finish).
                                        // Returns true when m_test() finished this step.

 private:
  std::function<void()> m_test;         // Thread entry point. The first time step() is called
                                        // after start(), this function will be called.
};

} // namespace thread_permuter
