#pragma once

#include "Thread.h"
#include <functional>
#include <string>
#include <exception>
#include <limits>

// An object of this type allows one to explore
// the possible results of running two or more
// functions simultaneously in different threads.
//
class ThreadPermuter
{
 public:
  using thi_type = ThreadIndex;
  using tests_type = utils::Vector<std::function<void()>, thi_type>;
  using threads_type = utils::Vector<thread_permuter::Thread, thi_type>;

  ThreadPermuter(std::function<void()> on_permutation_begin, tests_type const& tests, std::function<void(std::string const&)> on_permutation_end);
  ~ThreadPermuter();

  void set_limit(int limit) { m_limit = limit; }
  void run(std::string permutation = {}, bool continue_running = false, bool debug_on = false);

 private:
  threads_type m_threads;                                       // The functions, one for each thread, that need to be run.
  std::function<void()> m_on_permutation_begin;                 // This callback is called every time before a new permutation starts.
  std::function<void(std::string const&)> m_on_permutation_end; // This callback is called every time after all tests finished,
                                                                // once for each possible permutation.
  std::string m_permutation_string;                             // Records the permutation last executed by play().
  int m_limit = std::numeric_limits<int>::max();
};

#ifndef CWDEBUG
#define TP_ASSERT(x) assert(x)
#else
#define TP_ASSERT(x) \
  do \
  { \
    if (!(x)) \
    { \
      LIBCWD_TSD_DECLARATION; \
      if (LIBCWD_DO_TSD_MEMBER_OFF(libcwd::libcw_do)) \
        ASSERT(x); \
      else \
        throw PermutationFailure(#x, __FILE__, __LINE__); \
    } \
  } \
  while (0)
#endif

#include "ConditionVariable.h"
