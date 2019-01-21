#include "sys.h"
#include "ThreadPermuter.h"
#include "Permutation.h"
#include <iostream>

ThreadPermuter::ThreadPermuter(tests_type const& tests, std::function<void()> on_permutation_done)
  : m_threads(tests.begin(), tests.end()), m_on_permutation_done(on_permutation_done)
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
    // Play one permutation.
    permutation.play();
    // Notify that the program has finished.
    m_on_permutation_done();
  }
  while (permutation.next());   // Continue with the next permutation, if any.

  Dout(dc::notice|flush_cf, "All permutations finished.");

  for (thi_type thi(0); thi < end; ++thi)
    m_threads[thi].stop();
}
