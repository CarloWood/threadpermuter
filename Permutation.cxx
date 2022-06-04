#include "sys.h"
#include "Permutation.h"
#include "ConditionVariable.h"
#include "utils/log2.h"
#include <iostream>

namespace thread_permuter {

// Step thread thi and update m_blocked_threads and m_running_threads accordingly.
// Returns true if after this step the thread is still running (not blocked and not finished).
bool Permutation::step(thi_type thi, std::string& permutation_string)
{
  DoutEntering(dc::permutation, "Permutation::step(" << thi << ")");
  threads_set_type thm = index2mask(thi);
  // This should never happen because we'd never run this in the first place.
  // The most likely reason for asserting here is when you pass an illegal permutation to ThreadPermuter::run();
  // for example, when you changed the program and are still using an old permutation string.
  ASSERT((thm & ~m_blocked_threads & m_running_threads).any());
  permutation_string += '0' + thi.get_value();
  Thread& thread(m_threads[thi]);
  switch (thread.step(m_debug_on))
  {
    case yielding:
      m_blocked_threads.reset();
      break;
    case blocking:
      m_blocked_threads |= index2mask(thi);
      break;
    case blocking_with_progress:
      m_blocked_threads = index2mask(thi);
      break;
    case waiting:
      ASSERT((m_waiting_threads & index2mask(thi)).none());
      // Since we just unlocked a mutex, mark this always as progress (index2mask(thi) is going to be set again through m_waiting_threads at function exit).
      m_blocked_threads.reset();
      Dout(dc::permutation|continued_cf, "Adding thread " << thi << " to m_waiting_threads (" << m_waiting_threads << "). Result: ");
      m_waiting_threads |= index2mask(thi);
      Dout(dc::finish, m_waiting_threads);
      break;
    case notify_one:
    {
      ConditionVariable* cv = thread.condition_variable();
      if (cv->waiting_threads().none())
      {
        Dout(dc::permutation, "No waiting threads; ignoring notify_one()");
        break;
      }
      // Unblock all waiting threads and block all other threads for a single step.
      Dout(dc::permutation|continued_cf, "Setting m_blocked_threads (" << m_blocked_threads <<") to ~cv->waiting_threads(). Result: ");
      m_woken_threads = cv->waiting_threads();
      m_blocked_threads = ~cv->waiting_threads();
      Dout(dc::finish, m_blocked_threads);
      return false;
    }
    case woken:
    {
      ConditionVariable* cv = thread.condition_variable();
      Dout(dc::permutation|continued_cf, "Thread " << thi << " woke up. Setting m_waiting_threads to m_woken_threads (" << m_woken_threads << ") minus thread " << thi << ". Result: ");
      m_waiting_threads = m_woken_threads & ~index2mask(thi);
      Dout(dc::finish, m_waiting_threads);
      m_woken_threads.reset();
      m_blocked_threads.reset();
      break;
    }
    case notify_all:
    {
      ConditionVariable* cv = thread.condition_variable();
      if (cv->waiting_threads().none())
      {
        Dout(dc::permutation, "No waiting threads; ignoring notify_all()");
        break;
      }
      Dout(dc::permutation|continued_cf, "Removing " << cv->waiting_threads() << " from m_waiting_threads (" << m_waiting_threads <<") Result: ");
      m_waiting_threads &= ~cv->waiting_threads();
      Dout(dc::finish, m_waiting_threads);
      break;
    }
    case finished:
      // Reset bit thi when step() returns finished, which means that
      // that thread finished and is no longer running after this step.
      m_running_threads &= ~index2mask(thi);
      m_blocked_threads.reset();
      break;
    case failed:
      m_running_threads &= ~index2mask(thi);
      m_blocked_threads.reset();
      throw m_threads[thi].failure();
  }
  Dout(dc::permutation(m_waiting_threads.any()), "Add m_waiting_threads (" << m_waiting_threads <<") to m_blocked_threads.");
  m_blocked_threads |= m_waiting_threads;
  return (thm & ~m_blocked_threads & m_running_threads).any();
}

// Play back the recording.
void Permutation::play(std::string& permutation_string, bool run_complete)
{
  DoutEntering(dc::permutation, "Permutation::play(" << std::boolalpha << run_complete << ")");
  using namespace utils::bitset;

  thi_type const thread_end(m_threads.size());
  m_running_threads = index2mask(thread_end) - 1;       // Set all threads to running.
  m_blocked_threads.reset();                            // Nothing is blocked.
  m_waiting_threads.reset();                            // Nothing is waiting.
  m_woken_threads.reset();                              // Nothing was woken up temporarily.
  for (auto thi : m_steps)
    step(thi, permutation_string);
  // Complete the permutation by running all remaining threads till they are finished too, if so requested.
  if (run_complete && m_running_threads.any())
    complete(permutation_string);
}

// Run an incomplete permutation to completion.
void Permutation::complete(std::string& permuation_string)
{
  DoutEntering(dc::permutation, "Permutation::complete()");
  using namespace utils::bitset;

  // This may never happen.
  ASSERT(!m_running_threads.none());
  // First run all but the last running thread to completion while adding them to m_steps.
  while (!m_running_threads.is_single_bit())
  {
    threads_set_type yielding_threads = m_running_threads & ~m_blocked_threads;
    if (yielding_threads.none())
      DoutFatal(dc::core, "Dead locked (all still running threads are blocked)! While running: " << permuation_string);
    Index thi = yielding_threads.lssbi();
    m_steps.push_back(thi);
    m_blocked.push_back(m_blocked_threads);
    m_waiting.push_back(m_waiting_threads);
    m_woken.push_back(m_woken_threads);
    step(thi, permuation_string);
  }
  // Now there is only one running thread left.
  Index const last_thi = m_running_threads.lssbi();
  // Finished threads aren't blocked, are they?
  ASSERT((m_blocked_threads & ~m_running_threads).none());
  // Actually run that one to completion too, but don't add it to m_steps.
  do
  {
    if (m_running_threads == m_blocked_threads)
      DoutFatal(dc::core, "Dead locked (last running thread is blocked)! While running: " << permuation_string);
    step(last_thi, permuation_string);
  }
  while (!m_running_threads.none());
  // We shouldn't have reset m_running_threads though.
  m_running_threads = last_thi;

  int static count;
  if (++count % 1000 == 0 || count < 100)
    std::cout << "Completed: " << permuation_string << std::endl;
}

bool Permutation::next(int limit)
{
  DoutEntering(dc::permutation, "Permutation::next(" << limit << ")");
  Dout(dc::permutation, "Permutation before: " << *this);
  // Advance the permutation to the next.
  //
  // The algorithm here is to find the last step
  // whose thread index is smaller than one of
  // the still running threads and replace it
  // with that thread.
  //
  // For example, if m_steps contains the indices,
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
  int si = m_steps.size();
  while (--si >= limit)
    m_running_threads |= index2mask(m_steps[si]);
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
      m_waiting.resize(si + 1);         // Idem.
      m_woken.resize(si + 1);           // Idem.
      // Restore those values.
      m_blocked_threads = m_blocked[si];
      m_waiting_threads = m_waiting[si];
      m_woken_threads = m_woken[si];
      Dout(dc::permutation, "Permutation after: " << *this);
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
  m_waiting.clear();
  m_woken.clear();
  m_running_threads.reset();
  m_blocked_threads.reset();
  m_waiting_threads.reset();
  m_woken_threads.reset();
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
  os << "; running: " << permutation.m_running_threads << "; blocked: " << permutation.m_blocked_threads <<
    "; waiting: " << permutation.m_waiting_threads << "; woken: " << permutation.m_woken_threads;
  return os;
}

} // namespace thread_permuter
