#pragma once

#include "ThreadPermuter.h"
#include "utils/BitSet.h"
#include <vector>
#include <set>
#include <iosfwd>
#include <cstdint>
#include <string>

namespace thread_permuter {

class Permutation
{
 public:
  using thi_type = ThreadPermuter::thi_type;

  Permutation(ThreadPermuter::threads_type& threads) : m_threads(threads), m_running_threads(0), m_debug_on(false) { }

  bool step(thi_type thi, std::string& permutation_string);     // Play a single step on thread thi.
  void play(std::string& permutation_string, bool run_complete = true);
                                                                // Play the whole recorded permutation (if run_complete is false only play what is in m_steps).
  void complete(std::string& permutation_string);               // Complete a play()-ed permutation.
  bool next(int limit);                                         // Prepare for the next play(). Returns false when there isn't one.

  // Program a given permutation.
  void program(std::string const& steps);

 private:
  ThreadPermuter::threads_type& m_threads;      // A reference to the list of Thread objects.

  std::vector<thi_type> m_steps;                // Contains a list of thread indices that did a step;
  std::vector<threads_set_type> m_blocked;      // The blocked threads just prior to the corresponding step;
  std::vector<threads_set_type> m_waiting;      // The waiting threads just prior to the corresponding step;
  std::vector<threads_set_type> m_woken;        // The woken threads just prior to the corresponding step;
  threads_set_type m_running_threads;           // A list of thread indices that are still running after the last step in m_steps.
  threads_set_type m_blocked_threads;           // A list of thread indices that are currently blocked on trying to lock a mutex.
  threads_set_type m_waiting_threads;           // A list of thread indices that are currently waiting on a condition variable.
  threads_set_type m_woken_threads;             // A copy of m_waiting_threads made when notify_one is called.

 public:
  bool m_debug_on;

  friend std::ostream& operator<<(std::ostream& os, Permutation const& permutation);
};

} // namespace thread_permuter
