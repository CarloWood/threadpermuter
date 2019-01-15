#include "Permutation.h"
#include "utils/log2.h"
#include <iostream>

inline Permutation::mask_type index2mask(int i)
{
  return static_cast<Permutation::mask_type>(1) << i;
}

inline int mask2index(Permutation::mask_type mask)
{
  return utils::log2(mask);
}

void Permutation::add(int thi)
{
  m_running_threads |= index2mask(thi);
}

void Permutation::play()
{
  // Play back the recording.
  for (int thi : m_steps)
    m_threads[thi].step();
}

void Permutation::complete()
{
  // Run an incomplete permutation to completion.
  int const nr_running_threads = m_running_threads.size();
  // First run all but the last running thread to completion while adding them to m_steps.
  int count = 0;
  for (int thi : m_running_threads)
  {
    if (++count == nr_running_threads)
    {
      // Now there is only one running thread left.
      m_running_threads.clear();
      m_running_threads.insert(thi);
      // Actually run that one to completion too, but don't add it to m_steps.
      while (!m_threads[thi].step())
        continue;
      // We're done.
      return;
    }
    do
    {
      m_steps.push_back(thi);
    }
    while (!m_threads[thi].step());
  }
}

bool Permutation::next()
{
  // Advance the permutation to the next.
  int mi = *m_running_threads.begin();
  int si = m_steps.size() - 1;
  while (si >= 0)
  {
    m_running_threads.insert(m_steps[si]);
    if (m_steps[si] < mi)
    {
      m_steps[si] = mi;
      m_steps.resize(si + 1);
      return true;
    }
    --si;
  }
  return false;
}

std::ostream& operator<<(std::ostream& os, Permutation const& permutation)
{
  os << "Steps:";
  for (int s : permutation.m_steps)
    os << ' ' << s;
  os << "; Running:";
  for (int thi : permutation.m_running_threads)
    os << ' ' << thi;
  return os;
}
