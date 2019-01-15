#include "ThreadPermuter.h"
#include <iostream>

struct TestRun
{
};

void test0(int i)
{
  std::cout << "Calling test0(" << i << ")\n";
}

void test1(std::string s)
{
  std::cout << "Calling test1(\"" << s << "\")\n";
}

void test2(double d)
{
  std::cout << "Calling test2(" << d << ")\n";
}

void on_permutation_done(TestRun const& test_run)
{
  std::cout << "Calling on_permutation_done()\n";
}

int main()
{
  TestRun test_run;

  ThreadPermuter::tests_type tests =
  {
    []{test0(42);},
    []{test1("Hello World");},
    []{test2(3.14);}
  };

  ThreadPermuter tp(tests, [test_run]{ on_permutation_done(test_run); });

  tp.run();
}
