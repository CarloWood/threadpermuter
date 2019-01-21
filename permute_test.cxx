#include "sys.h"
#include "debug.h"
#include "ThreadPermuter.h"
#include <iostream>

struct TestRun
{
};

void test0(TestRun const& test_run)
{
  Dout(dc::notice, "Calling test0()");
}

void test1(TestRun const& test_run)
{
  Dout(dc::notice, "Calling test1()");
}

void test2(TestRun const& test_run)
{
  Dout(dc::notice, "Calling test2()");
}

void on_permutation_done(TestRun const& test_run)
{
  Dout(dc::notice|flush_cf, "Calling on_permutation_done()");
}

int main()
{
  Debug(NAMESPACE_DEBUG::init());

  TestRun test_run;

  ThreadPermuter::tests_type tests =
  {
    [test_run]{test0(test_run);},
    [test_run]{test1(test_run);},
    [test_run]{test2(test_run);}
  };

  ThreadPermuter tp(tests, [test_run]{ on_permutation_done(test_run); });

  tp.run();
}
