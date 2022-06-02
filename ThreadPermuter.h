#pragma once

#include "Thread.h"
#include "utils/Vector.h"
#include "utils/BitSet.h"
#include <functional>
#include <string>
#include <exception>

// VectorIndex category.
namespace vector_index_category {

// An index that specifies the test function (and corresponding thread).
// Initially defined by the first parameter passed to the constructor of ThreadPermuter.
struct thread_index;

} // namespace vector_index_category

// A type that can be used as index into ThreadPermuter::tests_type and ThreadPermuter::threads_type.
class ThreadIndex : public utils::VectorIndex<vector_index_category::thread_index>
{
  using utils::VectorIndex<vector_index_category::thread_index>::VectorIndex;
 public:
  // Allow bitset::Index to be used where ThreadIndex is required.
  ThreadIndex(utils::bitset::Index index) : utils::VectorIndex<vector_index_category::thread_index>(index()) { }
};

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

  void run(std::string permutation = {});

 private:
  threads_type m_threads;                                       // The functions, one for each thread, that need to be run.
  std::function<void()> m_on_permutation_begin;                 // This callback is called every time before a new permutation starts.
  std::function<void(std::string const&)> m_on_permutation_end; // This callback is called every time after all tests finished,
                                                                // once for each possible permutation.
  std::string m_permutation_string;                             // Records the permutation last executed by play().
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
