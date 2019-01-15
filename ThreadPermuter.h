#pragma once

#include "Thread.h"
#include <vector>
#include <functional>

// An object of this type allows one to explore
// the possible results of running two or more
// functions simultaneously in different threads.
//
class ThreadPermuter
{
 public:
  using tests_type = std::vector<std::function<void()>>;
  using threads_type = std::vector<thread_permuter::Thread>;

  ThreadPermuter(tests_type const& tests, std::function<void()> on_permutation_done);

  void run();

 private:
  threads_type m_threads;                               // The functions, one for each thread, that need to be run.
  std::function<void()> m_on_permutation_done;          // This callback is called every time after all tests finished,
                                                        // once for each possible permutation.
};
