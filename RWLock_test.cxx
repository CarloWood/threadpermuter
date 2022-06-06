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

  void test_rdlock_rdunlock();
  void test_wrlock_wrunlock();
  void test_wrlock_wr2rdlock_rdunlock();
  void test_rdlock_rd2wrlock_wrunlock();
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
  ASSERT(m_state == State::Unlocked && m_number_of_read_locks == 0);
}

void TestRun::test_rdlock_rdunlock()
{
  DoutEntering(dc::notice|flush_cf, "TestRun::test_rdlock_rdunlock()");

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

  Dout(dc::notice|flush_cf, "Leaving TestRun::test_rdlock_rdunlock()");
}

void TestRun::test_wrlock_wrunlock()
{
  DoutEntering(dc::notice|flush_cf, "TestRun::test_wrlock_wrunlock()");

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

  Dout(dc::notice|flush_cf, "Leaving TestRun::test_wrlock_wrunlock()");
}

void TestRun::test_wrlock_wr2rdlock_rdunlock()
{
  DoutEntering(dc::notice|flush_cf, "TestRun::test_wrlock_wr2rdlock_rdunlock()");

  Dout(dc::notice, "Calling wrlock()");
  m_lock.wrlock();
  TP_ASSERT(m_state == State::Unlocked);
  TP_ASSERT(m_number_of_read_locks == 0);
  Dout(dc::always, "Setting m_state @ " << (void*)&m_state << " to State::Writelocked");
  m_state = State::WriteLocked;

  Dout(dc::notice, "Calling wr2rdlock()");
  TP_ASSERT(m_state == State::WriteLocked);
  Dout(dc::always, "Setting m_state @ " << (void*)&m_state << " to State::Readlocked");
  m_state = State::ReadLocked;
  ++m_number_of_read_locks;
  m_lock.wr2rdlock();
  TP_ASSERT(m_state == State::ReadLocked);

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

  Dout(dc::notice|flush_cf, "Leaving TestRun::test_wrlock_wr2rdlock_rdunlock()");
}

void TestRun::test_rdlock_rd2wrlock_wrunlock()
{
  DoutEntering(dc::notice|flush_cf, "TestRun::test_rdlock_rd2wrlock_wrunlock()");

  for (;;)
  {
    Dout(dc::notice, "Calling rdlock()");
    m_lock.rdlock();
    TP_ASSERT(m_state != State::WriteLocked);
    TP_ASSERT((m_number_of_read_locks > 0) == (m_state == State::ReadLocked));
    Dout(dc::always, "Setting m_state @ " << (void*)&m_state << " to State::Readlocked");
    m_state = State::ReadLocked;
    ++m_number_of_read_locks;

    Dout(dc::notice, "Calling rd2wrlock()");
    ASSERT(m_state == State::ReadLocked);
    try
    {
      m_lock.rd2wrlock();
      break;
    }
    catch (std::exception const&)
    {
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
      Dout(dc::notice, "Calling rd2wryield()");
      m_lock.rd2wryield();
      // Instead of going to the top and try again; abort (do nothing)!
      return;
    }
  }
  TP_ASSERT(m_state == State::ReadLocked);
  TP_ASSERT(m_number_of_read_locks == 1);
  Dout(dc::always, "Setting m_state @ " << (void*)&m_state << " to State::Writelocked");
  m_state = State::WriteLocked;
  --m_number_of_read_locks;

  TP_ASSERT(m_state == State::WriteLocked);
  Dout(dc::always, "Setting m_state @ " << (void*)&m_state << " to State::Unlocked");
  m_state = State::Unlocked;
  Dout(dc::notice, "Calling wrunlock()");
  m_lock.wrunlock();

  Dout(dc::notice|flush_cf, "Leaving TestRun::test_rdlock_rd2wrlock_wrunlock()");
}

int main()
{
  Debug(NAMESPACE_DEBUG::init());
  Dout(dc::notice, "Entering main()");

  TestRun test_run;

  ThreadPermuter::tests_type tests =
  {
//    [&test_run]{ test_run.test_wrlock_wrunlock(); },
//    [&test_run]{ test_run.test_rdlock_rdunlock(); },
//    [&test_run]{ test_run.test_wrlock_wr2rdlock_rdunlock(); },
    [&test_run]{ test_run.test_rdlock_rd2wrlock_wrunlock(); },
    [&test_run]{ test_run.test_rdlock_rd2wrlock_wrunlock(); },
    [&test_run]{ test_run.test_rdlock_rd2wrlock_wrunlock(); }
  };

  ThreadPermuter tp(
      [&]{ test_run.on_permutation_begin(); },
      tests,
      [&](std::string const& permutation_string){ test_run.on_permutation_end(permutation_string); });

//  tp.set_limit(18);
//  tp.run();
  tp.run("011", true);

  Dout(dc::notice, "Success!");
}
