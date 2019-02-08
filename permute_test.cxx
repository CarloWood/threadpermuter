#include "sys.h"
#include "debug.h"
#include "ThreadPermuter.h"
#include <iostream>

struct TestRun
{
  thread_permuter::Mutex m_mutex;
  int x;

  void on_permutation_begin();
  void on_permutation_end(std::string const& permutation_string);
};

void TestRun::on_permutation_begin()
{
  DoutEntering(dc::notice|flush_cf, "on_permutation_begin()");
  x = 1;
}

void TestRun::on_permutation_end(std::string const& permutation_string)
{
  DoutEntering(dc::notice|flush_cf, "on_permutation_end(\"" << permutation_string << "\")");
  Dout(dc::notice|flush_cf, "Result: " << x);
}

void test0(TestRun& test_run)
{
  DoutEntering(dc::notice|flush_cf, "test0()");

  test_run.m_mutex.lock();
  TPY;

  Dout(dc::notice, "Adding 7");
  test_run.x += 7;
  TPY;

  test_run.m_mutex.unlock();
  TPY;

  Dout(dc::notice|flush_cf, "Leaving test0()");
}

void test1(TestRun& test_run)
{
  DoutEntering(dc::notice|flush_cf, "test1()");

  test_run.m_mutex.lock();
  TPY;

  Dout(dc::notice, "Multiplying by 3");
  test_run.x *= 3;
  TPY;

  test_run.m_mutex.unlock();
  TPY;

  Dout(dc::notice|flush_cf, "Leaving test1()");
}

void test2(TestRun& test_run)
{
  DoutEntering(dc::notice|flush_cf, "test2()");

  test_run.m_mutex.lock();
  TPY;

  Dout(dc::notice, "Modulo 5");
  test_run.x %= 5;
  TPY;

  test_run.m_mutex.unlock();
  TPY;

  Dout(dc::notice|flush_cf, "Leaving test2()");
}

int main()
{
  Debug(NAMESPACE_DEBUG::init());

  TestRun test_run;

  ThreadPermuter::tests_type tests =
  {
    [&test_run]{ test0(test_run); },
    [&test_run]{ test1(test_run); },
    [&test_run]{ test2(test_run); }
  };

  ThreadPermuter tp(
      [&]{ test_run.on_permutation_begin(); },
      tests,
      [&](std::string const& permutation_string){ test_run.on_permutation_end(permutation_string); });

  tp.run();
}
