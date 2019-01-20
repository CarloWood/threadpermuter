#pragma once

#include "ThreadPermuter.h"
#include "utils/BitSet.h"
#include <vector>
#include <set>
#include <iosfwd>
#include <cstdint>

class Permutation
{
 public:
  using mask_type = uint32_t;
  using threads_set_type = utils::BitSet<mask_type>;
  using thi_type = ThreadPermuter::thi_type;

  Permutation(ThreadPermuter::threads_type& threads) : m_threads(threads), m_running_threads(0) { }

  void add(thi_type thi);

  void play(bool run_complete = true);          // Play the whole recorded permutation (if run_complete is false only play what is in m_steps).
  void complete();                              // Complete a play()-ed permutation.
  bool next();                                  // Prepare for the next play(). Returns false when there isn't one.

 private:
  ThreadPermuter::threads_type& m_threads;      // A reference to the list of Thread objects.

  std::vector<thi_type> m_steps;                // Contains a list of thread indexes that did a step;
  threads_set_type m_running_threads;           // A list of thread indexes that are still running after the last step in m_steps.

  friend std::ostream& operator<<(std::ostream& os, Permutation const& permutation);
};
