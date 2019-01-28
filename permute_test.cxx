#include "sys.h"
#include "debug.h"
#include "ThreadPermuter.h"
#include <iostream>

struct TestRun
{
  std::mutex m_mutex;
  int x;
  std::string m_permutation;

  void lock(char t)
  {
    DoutEntering(dc::notice|flush_cf|continued_cf, "lock()... ");
    bool blocked = false;
    while (!m_mutex.try_lock())
    {
      Dout(dc::notice|flush_cf, "Blocked on mutex.");
      m_permutation += 'b';
      TPB;
      blocked = true;
    }
    if (blocked)
      m_permutation += t;
    Dout(dc::finish|flush_cf, "locked");
  }

  void unlock()
  {
    Dout(dc::notice|flush_cf, "unlock()");
    m_mutex.unlock();
  }

  void on_permutation_begin();
  void on_permutation_end(std::string const& permutation_string);
};

void TestRun::on_permutation_begin()
{
  DoutEntering(dc::notice|flush_cf, "on_permutation_begin()");
  x = 1;
  m_permutation = "";
}

void TestRun::on_permutation_end(std::string const& permutation_string)
{
  DoutEntering(dc::notice|flush_cf, "on_permutation_end(\"" << permutation_string << "\")");
  Dout(dc::notice|flush_cf, "Result: " << x);
}

void test0(TestRun& test_run)
{
  DoutEntering(dc::notice|flush_cf, "test0()");

  test_run.m_permutation += '0';
  test_run.lock('0');
  TPY;

  test_run.m_permutation += '0';
  Dout(dc::notice, "Adding 7");
  test_run.x += 7;
  TPY;

  test_run.m_permutation += '0';
  test_run.unlock();
  TPY;

  test_run.m_permutation += "0f";
  Dout(dc::notice|flush_cf, "Leaving test0()");
}

void test1(TestRun& test_run)
{
  DoutEntering(dc::notice|flush_cf, "test1()");

  test_run.m_permutation += '1';
  test_run.lock('1');
  TPY;

  test_run.m_permutation += '1';
  Dout(dc::notice, "Multiplying by 3");
  test_run.x *= 3;
  TPY;

  test_run.m_permutation += '1';
  test_run.unlock();
  TPY;

  test_run.m_permutation += "1f";
  Dout(dc::notice|flush_cf, "Leaving test1()");
}

void test2(TestRun& test_run)
{
  DoutEntering(dc::notice|flush_cf, "test2()");

  test_run.m_permutation += '2';
  test_run.lock('2');
  TPY;

  test_run.m_permutation += '2';
  Dout(dc::notice, "Modulo 5");
  test_run.x %= 5;
  TPY;

  test_run.m_permutation += '2';
  test_run.unlock();
  TPY;

  test_run.m_permutation += "2f";
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
