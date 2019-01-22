#include "sys.h"
#include "ThreadPermuter.h"
#include "Permutation.h"
#include <iostream>

using namespace thread_permuter;

ThreadPermuter::ThreadPermuter(
    std::function<void()> on_permutation_begin,
    tests_type const& tests,
    std::function<void()> on_permutation_end)
  : m_threads(tests.begin(), tests.end()), m_on_permutation_begin(on_permutation_begin), m_on_permutation_end(on_permutation_end)
{
}

void ThreadPermuter::run()
{
  Permutation permutation(m_threads);

  // Start all threads.
  thi_type const end(m_threads.size());
  for (thi_type thi(0); thi < end; ++thi)
    m_threads[thi].start();

  do
  {
    // Notify that we start a new program.
    m_on_permutation_begin();
    // Play one permutation.
    permutation.play();
    // Notify that the program has finished.
    m_on_permutation_end();
  }
  while (permutation.next());   // Continue with the next permutation, if any.

  Dout(dc::notice|flush_cf, "All permutations finished.");

  for (thi_type thi(0); thi < end; ++thi)
    m_threads[thi].stop();
}
