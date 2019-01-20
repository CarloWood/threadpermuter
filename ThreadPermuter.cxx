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

  // Total number of threads.
  int const n = m_threads.size();

  // Start all threads and initialize an empty Permutation (no steps).
  thi_type end(n);
  for (thi_type thi(0); thi < end; ++thi)
  {
    m_threads[thi].start();
    permutation.add(thi);
  }

  do
  {
    permutation.play();
    m_on_permutation_done();
  }
  while (permutation.next());
}
