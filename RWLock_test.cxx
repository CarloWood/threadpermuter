#include "sys.h"
#include "debug.h"
#include "ThreadPermuter.h"
#define DEBUG_RWSPINLOCK_THREADPERMUTER
#include "threadsafe/AIReadWriteSpinLock.h"

enum class State {
  Unlocked,
  ReadLocked,
  WriteLocked
};

#ifdef CWDEBUG
std::ostream& operator<<(std::ostream& os, State state)
{
  switch (state)
  {
    case State::Unlocked:
      os << "Unlocked";
      break;
    case State::ReadLocked:
      os << "ReadLocked";
      break;
    case State::WriteLocked:
      os << "WriteLocked";
      break;
  }
  return os;
}
#endif

struct TestRun
{
  AIReadWriteSpinLock m_lock;
  State m_state;
  int m_number_of_read_locks;

  void on_permutation_begin();
  void on_permutation_end(std::string const& permutation_string);

  void test0();
  void test1();
  void test2();
};

void TestRun::on_permutation_begin()
{
  DoutEntering(dc::notice|flush_cf, "on_permutation_begin()");
  Dout(dc::always, "Setting m_state @ " << (void*)&m_state << " to State::Unlocked");
  m_state = State::Unlocked;
  m_number_of_read_locks = 0;
}

void TestRun::on_permutation_end(std::string const& permutation_string)
{
  DoutEntering(dc::notice|flush_cf, "on_permutation_end(\"" << permutation_string << "\")");
  Dout(dc::notice|flush_cf, "Result: " << m_state);
//  ASSERT(m_state == State::Unlocked && m_number_of_read_locks == 0);
}

void TestRun::test0()
{
  DoutEntering(dc::notice|flush_cf, "TestRun::test1()");

  Dout(dc::notice, "Calling wrlock()");
  m_lock.wrlock();
  TP_ASSERT(m_state == State::Unlocked);
  TP_ASSERT(m_number_of_read_locks == 0);
  Dout(dc::always, "Setting m_state @ " << (void*)&m_state << " to State::Writelocked");
  m_state = State::WriteLocked;

  TP_ASSERT(m_state == State::WriteLocked);
  Dout(dc::always, "Setting m_state @ " << (void*)&m_state << " to State::Unlocked");
  m_state = State::Unlocked;
  Dout(dc::notice, "Calling wrunlock()");
  m_lock.wrunlock();

  Dout(dc::notice|flush_cf, "Leaving TestRun::test1()");
}

void TestRun::test1()
{
  DoutEntering(dc::notice|flush_cf, "TestRun::test0()");

  Dout(dc::notice, "Calling rdlock()");
  m_lock.rdlock();
  TP_ASSERT(m_state != State::WriteLocked);
  TP_ASSERT((m_number_of_read_locks > 0) == (m_state == State::ReadLocked));
  Dout(dc::always, "Setting m_state @ " << (void*)&m_state << " to State::Readlocked");
  m_state = State::ReadLocked;
  ++m_number_of_read_locks;

  TP_ASSERT(m_number_of_read_locks > 0);
  if (--m_number_of_read_locks == 0)
  {
    Dout(dc::always, "Setting m_state @ " << (void*)&m_state << " to State::Unlocked");
    m_state = State::Unlocked;
  }
  else
    TP_ASSERT(m_state == State::ReadLocked);
  Dout(dc::notice, "Calling rdunlock()");
  m_lock.rdunlock();

  Dout(dc::notice|flush_cf, "Leaving TestRun::test0()");
}

int main()
{
  Debug(NAMESPACE_DEBUG::init());
  Dout(dc::notice, "Entering main()");

  TestRun test_run;

  ThreadPermuter::tests_type tests =
  {
    [&test_run]{ test_run.test0(); },
    [&test_run]{ test_run.test1(); },
    [&test_run]{ test_run.test1(); }
  };

  ThreadPermuter tp(
      [&]{ test_run.on_permutation_begin(); },
      tests,
      [&](std::string const& permutation_string){ test_run.on_permutation_end(permutation_string); });

  tp.run();

  Dout(dc::notice, "Success!");
}
