#include "sys.h"
#include "ThreadPermuter.h"
#include "Permutation.h"
#include <iostream>

using namespace thread_permuter;

ThreadPermuter::ThreadPermuter(
    std::function<void()> on_permutation_begin,
    tests_type const& tests,
    std::function<void(std::string const&)> on_permutation_end)
  : m_threads(tests.begin(), tests.end()), m_on_permutation_begin(on_permutation_begin), m_on_permutation_end(on_permutation_end)
{
}

ThreadPermuter::~ThreadPermuter()
{
}

void ThreadPermuter::run(std::string single_permutation)
{
  Permutation permutation(m_threads);

  // Start all threads.
  thi_type const end(m_threads.size());
  for (thi_type thi(0); thi < end; ++thi)
    m_threads[thi].start('0' + thi.get_value());

  if (single_permutation.empty())
  {
    do
    {
      // Notify that we start a new program.
      m_on_permutation_begin();
      // Play one permutation.
      m_permutation_string.clear();
      permutation.play(m_permutation_string);
      // Notify that the program has finished.
      m_on_permutation_end(m_permutation_string);
    }
    while (permutation.next());   // Continue with the next permutation, if any.
    Dout(dc::notice|flush_cf, "All permutations finished.");
  }
  else
  {
    permutation.program(single_permutation);
    m_on_permutation_begin();
    m_permutation_string.clear();
    permutation.play(m_permutation_string);
    m_on_permutation_end(m_permutation_string);
  }

  for (thi_type thi(0); thi < end; ++thi)
    m_threads[thi].stop();
}
