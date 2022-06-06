#include "sys.h"
#include "ThreadPermuter.h"
#include "Permutation.h"
#include <iostream>

using namespace thread_permuter;

ThreadPermuter::ThreadPermuter(
    std::function<void()> on_permutation_begin,
    tests_type const& tests,
    std::function<void(std::string const&)> on_permutation_end)
  : m_on_permutation_begin(on_permutation_begin), m_on_permutation_end(on_permutation_end)
{
  std::vector<std::pair<std::function<void()>, thi_type>> vp;
  for (thi_type thi = tests.ibegin(); thi != tests.iend(); ++thi)
    vp.emplace_back(tests[thi], thi);
  utils::Vector<thread_permuter::Thread, thi_type> tmp(vp.begin(), vp.end());
  m_threads = std::move(tmp);
}

ThreadPermuter::~ThreadPermuter()
{
}

void ThreadPermuter::run(std::string single_permutation, bool continue_running, bool debug_on)
{
  Permutation permutation(m_threads);

  bool debug_off = !debug_on && (single_permutation.empty() || continue_running);

  // Start all threads.
  thi_type const end(m_threads.size());
  for (thi_type thi(0); thi < end; ++thi)
    m_threads[thi].start('0' + thi.get_value(), debug_off);

  if (!single_permutation.empty())
    permutation.program(single_permutation);

  if (single_permutation.empty() || continue_running)
  {
    Debug(libcw_do.off());
    int number_of_permutations = 0;
    for (;;)
    {
      // Notify that we start a new program.
      m_on_permutation_begin();
      // Play one permutation.
      m_permutation_string.clear();

      bool failed = false;
      try
      {
        permutation.play(m_permutation_string);
        ++number_of_permutations;
      }
      catch (PermutationFailure const& error)
      {
        Debug(libcw_do.on());
        Dout(dc::notice, "Permutation \"" << m_permutation_string << "\" failed assertion " << error.message() << ".");
        failed = true;  // Cause permutation to run again with debug output turned on.
        permutation.m_debug_on = true;
      }

      // Notify that the program has finished.
      m_on_permutation_end(m_permutation_string);

      // restrict variations to the first m_limit steps.
      if (!failed && !permutation.next(m_limit))   // Continue with the next permutation, if any.
        break;
    }
    Debug(libcw_do.on());
    if (m_limit == std::numeric_limits<int>::max())
      Dout(dc::notice|flush_cf, "All " << number_of_permutations << " permutations finished.");
    else
      Dout(dc::notice|flush_cf, "Completed " << number_of_permutations << " number of permutations.");
  }
  else
  {
    m_on_permutation_begin();
    m_permutation_string.clear();
    permutation.play(m_permutation_string);
    m_on_permutation_end(m_permutation_string);
  }

  for (thi_type thi(0); thi < end; ++thi)
    m_threads[thi].stop();
}
