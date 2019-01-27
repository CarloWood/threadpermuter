#include "sys.h"
#include "Permutation.h"
#include "utils/log2.h"
#include <iostream>

namespace thread_permuter {

// Allow conversion from thi_type to threads_set_type.
inline Permutation::threads_set_type index2mask(Permutation::thi_type thi)
{
  return Permutation::threads_set_type(static_cast<Permutation::mask_type>(1) << thi.get_value());
}

// Step thread thi and update m_blocked_threads and m_running_threads accordingly.
// Returns true if after this step the thread is still running (not blocked and not finished).
bool Permutation::step(thi_type thi)
{
  DoutEntering(dc::notice, "Permutation::step(" << thi << ")");
  threads_set_type thm = index2mask(thi);
  // This should never happen because we'd never run this in the first place.
  ASSERT((thm & ~m_blocked_threads & m_running_threads).any());
  switch (m_threads[thi].step())
  {
    case yielding:
      m_blocked_threads.reset();
      break;
    case blocking:
      m_blocked_threads |= index2mask(thi);
      break;
    case finished:
      // Reset bit thi when step() returns finished, which means that
      // that thread finished and is no longer running after this step.
      m_running_threads &= ~index2mask(thi);
      m_blocked_threads.reset();
      break;
  }
  return (thm & ~m_blocked_threads & m_running_threads).any();
}

// Play back the recording.
void Permutation::play(bool run_complete)
{
  DoutEntering(dc::notice, "Permutation::play(" << run_complete << ")");
  using namespace utils::bitset;

  thi_type const thread_end(m_threads.size());
  m_running_threads = index2mask(thread_end) - 1;       // Set all threads to running.
  m_blocked_threads.reset();                            // Nothing is blocked.
  for (auto thi : m_steps)
    step(thi);
  // Complete the permutation by running all remaining threads till they are finished too, if so requested.
  if (run_complete)
    complete();
}

// Run an incomplete permutation to completion.
void Permutation::complete()
{
  DoutEntering(dc::notice, "Permutation::complete()");
  using namespace utils::bitset;

  // This may never happen.
  ASSERT(!m_running_threads.none());
  // First run all but the last running thread to completion while adding them to m_steps.
  while (!m_running_threads.is_single_bit())
  {
    threads_set_type yielding_threads = m_running_threads & ~m_blocked_threads;
    if (yielding_threads.none())
      DoutFatal(dc::core, "Dead locked!");
    Index thi = yielding_threads.lssbi();
    m_steps.push_back(thi);
    m_blocked.push_back(m_blocked_threads);
    step(thi);
  }
  // Now there is only one running thread left.
  Index const last_thi = m_running_threads.lssbi();
  // Actually run that one to completion too, but don't add it to m_steps.
  do
  {
    if (m_running_threads == m_blocked_threads)
      DoutFatal(dc::core, "Dead locked!");
    step(last_thi);
  }
  while (!m_running_threads.none());
  // We shouldn't have reset m_running_threads though.
  m_running_threads = last_thi;
}

bool Permutation::next()
{
  DoutEntering(dc::notice, "Permutation::next()");
  Dout(dc::notice, "Permutation before: " << *this);
  // Advance the permutation to the next.
  //
  // The algorithm here is to find the last step
  // whose thread index is smaller than one of
  // the still running threads and replace it
  // with that thread.
  //
  // For example, if m_steps contains the indexes,
  // 1, 2, 1  and at that point the running threads
  // are 0 (1 and 2 finished) then the last '1' in
  // m_steps is not replaced because 0 (the running
  // thread) is not larger than 1. The '2' is not
  // replaced because neither 0 nor 1 (which is still
  // running at that point!) is larger than 2. But
  // the first '1' in m_steps will be replaced with
  // 2 because it is larger than 1 and still running
  // at that point.
  //
  // Another example, the list
  //
  // si: 0 1 2 3 4 5 6     0 1 2   <-- m_steps[] index.
  //     3 4 2 5 4 1 0 --> 3 4 4
  //         ^____
  //              \.
  // because the '2' is the first (scanning from
  // the right) that has a larger index on its
  // right, and it is replaced with 4, because
  // that is the smaller number on its right that
  // is still larger than 2.

  // Let si be the index into m_steps and start
  // scanning from the right-most position.
  int si = m_steps.size() - 1;
  while (si >= 0)
  {
    thi_type thi = m_steps[si];                                                         // As per the above example, assume thi == 2 now.
    threads_set_type thm = index2mask(thi);                                             //               thm = 00000100
    threads_set_type blocked_threads = m_blocked[si];
    // When scanning further to the left, this thread is now also running.
    m_running_threads |= thm;                                                           // m_running_threads = 00110111
    // Get a copy of m_running_threads without thi.
    threads_set_type hi_rts = m_running_threads ^ thm;                                  //            hi_rts = 00110011
    // Do not consider currently blocked threads.                                                                ^^
    hi_rts &= ~blocked_threads;                                                         //                       ||
    if (hi_rts > thm)   // Is there a running thread with an index larger than m_steps[si]?     // Yes, because  \\__ we have bits here.
    {
      // We found the step that needs to be incremented (si).
      // Also remove the thi's that are lower than thi.
      hi_rts &= ~(thm - 1);                                                             //            hi_rts = 00110000
      // Then increment m_steps[si] to the set index above thi,                                                   ^
      // the index of the least significant set bit in hi_rts.                                                    |
      m_steps[si] = hi_rts.lssbi();                                                     // m_steps[si] = 4 -------'
      m_steps.resize(si + 1);                                                           // Resize m_steps to just "3 4 4".
      m_blocked.resize(si + 1);         // Note that m_blocked is still correct because it refers to what happened *before* this step.
      Dout(dc::notice, "Permutation after: " << *this);
      return true;
    }
    --si;
  }
  return false;
}

void Permutation::program(std::string const& steps)
{
  m_steps.clear();
  m_blocked.clear();
  m_running_threads.reset();
  m_blocked_threads.reset();
  for (auto c : steps)
  {
    size_t thread_id = c - '0';
    m_steps.push_back(ThreadIndex(thread_id));
  }
}

std::ostream& operator<<(std::ostream& os, Permutation const& permutation)
{
  os << "Steps:";
  for (auto s : permutation.m_steps)
    os << ' ' << s;
  os << "; running: " << permutation.m_running_threads << "; blocked: " << permutation.m_blocked_threads;
  return os;
}

} // namespace thread_permuter
