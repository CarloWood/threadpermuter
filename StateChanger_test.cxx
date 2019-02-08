#include "sys.h"
#include "debug.h"
#include "ThreadPermuter.h"
#include <iostream>
#include <atomic>


// We have the demand that the _last_ value passed to state.replace()
// is also passed to state.set_if(), and that that is not ignored
// inside state_if().
//
// Here we assume that each thread does the equivalent of the following:
//
// state.replace(1);                    // Replace a shared atomic with the value 1.
// TemporalState ts(state);             // Copy the value into a TemporalState object (value can be 1 or already be overwritten by another thread, lets say 2).
// if (ts == 1)                         // Tests if the copied value is the value we used.
//   state.set_if(ts);                  // Calls a function that enters a critical area where state 1 is executed or not.

// Clearly, when there is no other thread at all, then this
// calls set_if(1) and that will execute it.

// Also, if any thread changes state before we read it back into ts,
// then we never call set_if(). This is correct because in that case
// there was a call to state.replace() after out call to state.replace(1)
// and therefore 1 wasn't the last value passed.
//
// Because there is always one thread that calls state.replace() last,
// that thread will copy its own value and call state.set_if() with
// it. But we need to make sure that that call is never ignored.
//
// The construction of TemporalState can be divided into two parts:
// 1) Something that we still have to define. Lets represent this with a function B().
// 2) The actual copying of the shared atomic state value.
//
// The call to set_if() can be written as:
//
// if (something that we still have to define)  // Lets represent this by calling a function E().
//   /* Actually use/execute the argument. */
//
// All together we get:
//
// A) state = x;
// B) TemporalState ts; B(&ts, x);
// C) ts = state;
// D) if (ts == x)
// E)   if (E(&ts, x))
// F)     final = ts;
//
// The demands of E() can then be derived as follows:
// It must return false on thread 0 when thread 0 passes D,
// but another thread passed A later than thread 0 passed A
// and thread 0 calls set_f() after that other thread.
//
//      Thread 0                        Thread 1
//      state = 0;
//      TemporalState ts; B(&ts, 0);
//      ts = state;
//                                      state = 1
//      if (ts == 0)
//                                      TemporalState ts; B(&ts, 1);
//                                      ts = state;
//                                      if (ts == 1)
//                                        if (E(&ts, x))
//                                          final = ts;
//        if (E(&ts, 0)) // Must return false.

// From which could be concluded that E(&ts, 0) must return false
// when B(&ts, 1) was called after B(&ts, 0).
//
// But then there is this case:
//
//      Thread 0                        Thread 1
//                                      state = 1
//      state = 0;
//      TemporalState ts; B(&ts, 0);
//                                      TemporalState ts; B(&ts, 1);
//                                      ts = state;
//                                      if (ts == 1) // Ends here because ts will be 0.
//      ts = state;
//      if (ts == 0)
//        if (E(&ts, 0)) // Must return true.
//          final = ts;

// From which we can conclude that E(&ts, 0) must return true
// even though B(&ts, 1) was called after B(&ts, 0).
// The only difference seems to be that this time the value
// of state is 0 instead of one.

using namespace thread_permuter;

class StateChanger;

class TemporalState
{
 private:
  friend class StateChanger;
  int m_temporal_state;

 public:
  TemporalState(StateChanger& state_changer);

  operator int() const { return m_temporal_state; }
};

class StateChanger
{
 private:
  std::atomic_int m_state;
//  std::atomic<TemporalState const*> m_last;

 public:
  StateChanger() : m_state(-2) { }

  void replace(int state)
  {
    Dout(dc::notice|flush_cf, "thread " << Thread::name() << ": Release: changing atomic to " << state);
    m_state.store(state, std::memory_order_release);
  }

 private:
  friend class TemporalState;
  int copy_state(TemporalState const* ptr)
  {
    DoutEntering(dc::notice|flush_cf, "thread " << Thread::name() << ": copy_state(" << ptr << ")");
//    Dout(dc::notice|flush_cf, "thread " << Thread::name() << ": Writing " << ptr << " to m_last.");
//    m_last.store(ptr);
    Dout(dc::notice|flush_cf, "thread " << Thread::name() << ": Acquire: copying atomic value " << m_state);
    return m_state.load(std::memory_order_acquire);
  }

 protected:
  bool valid(TemporalState const* ptr)
  {
    DoutEntering(dc::notice|flush_cf, "thread " << Thread::name() << ": valid(" << ptr << ")");
//    Dout(dc::notice|flush_cf, "thread " << Thread::name() << ": Replacing m_last (" << m_last << ") with nullptr iff it is equal to " << ptr << " (not valid when not equal).");
    return *ptr == m_state;
  }
};

TemporalState::TemporalState(StateChanger& state_changer) : m_temporal_state(state_changer.copy_state(this))
{
}

class UserObj : public StateChanger
{
 private:
  Mutex m_final_state_mutex;
  int m_final_state;
  int m_state_expected;

 public:
  UserObj() : m_final_state(-1), m_state_expected(0) { }
  void on_permutation_begin();
  void on_permutation_end(std::string const& permutation);

  void set_expected(int state)
  {
    Dout(dc::notice|flush_cf, "thread " << Thread::name() << ": Setting m_state_expected to " << state);
    m_state_expected = state;
  }

  friend std::ostream& operator<<(std::ostream& os, UserObj const& user_obj);

  void set_if(TemporalState const& state)
  {
    DoutEntering(dc::notice|flush_cf, "thread " << Thread::name() << ": Calling set_if(" << state << ")");
    std::lock_guard<Mutex> lck(m_final_state_mutex);
    TPY;
    if (valid(&state))
    {
      TPY;
      Dout(dc::notice|flush_cf, "Setting m_final_state to " << state);
      m_final_state = state;
    }
    else
      Dout(dc::notice|flush_cf, "Ignoring call to set_if() because state is invalid.");
  }
};

void UserObj::on_permutation_begin()
{
  DoutEntering(dc::notice|flush_cf, "on_permutation_begin()");
  m_final_state = -1;
  m_state_expected = -1;
  Dout(dc::notice|flush_cf, "Before: " << *this);
}

void UserObj::on_permutation_end(std::string const& permutation)
{
  DoutEntering(dc::notice|flush_cf, "on_permutation_end(\"" << permutation << "\")");
  Dout(dc::notice|flush_cf, "Result: " << *this);
  ASSERT(m_final_state == m_state_expected);
}

void thread0(UserObj& state)
{
  state.replace(0);                             // Release
  state.set_expected(0);
  TPY;

  TemporalState ts(state);                      // Lock and acquire.
  TPY;

  if (ts == 0)                                  // If empty, stop.
  {
    TPY;
    state.set_if(ts);
  }
}

void thread1(UserObj& state)
{
  state.replace(1);
  state.set_expected(1);
  TPY;

  TemporalState ts(state);
  TPY;

  if (ts == 1)                                  // If not empty, start.
  {
    TPY;
    state.set_if(ts);
  }
}

void thread2(UserObj& state)
{
  state.replace(2);
  state.set_expected(2);
  TPY;

  TemporalState ts(state);
  TPY;

  if (ts == 2)                                  // If not empty, start.
  {
    TPY;
    state.set_if(ts);
  }
}

int main()
{
  Debug(NAMESPACE_DEBUG::init());

  for (int s1 = 0; s1 < 2; ++s1)
    for (int s2 = 0; s2 < 2; ++s2)
    {
      UserObj state;

      ThreadPermuter::tests_type tests =
      {
        [&]{ thread0(state); },
        [&]{ thread1(state); },
        [&]{ thread2(state); }
      };

      ThreadPermuter tp(
          [&]{ state.on_permutation_begin(); },
          tests,
          [&](std::string const& permutation){ state.on_permutation_end(permutation); }
      );

      tp.run();

      Dout(dc::notice|flush_cf, "After: " << state);
    }
}

std::ostream& operator<<(std::ostream& os, UserObj const& user_obj)
{
  os << "Final state: " << user_obj.m_final_state << "; expected: " << user_obj.m_state_expected;
  return os;
}
