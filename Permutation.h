#pragma once

#include "ThreadPermuter.h"
#include <vector>
#include <set>
#include <iosfwd>
#include <cstdint>

class Permutation
{
 public:
  using mask_type = uint32_t;

  Permutation(ThreadPermuter::threads_type& threads) : m_threads(threads) { }

  void add(int thi);

  void play();                                          // Play the whole recorded permutation.
  void complete();                                      // Complete a play()-ed permutation.
  bool next();                                          // Prepare for the next play(). Returns false when there isn't one.

 private:
  ThreadPermuter::threads_type& m_threads;              // A reference to the list of Thread objects.

  std::vector<int> m_steps;                             // Contains a list of thread indexes that did a step;
  mask_type m_running_threads;                          // A list of thread indexes that are still running after the last step in m_steps.

  friend std::ostream& operator<<(std::ostream& os, Permutation const& permutation);
};
