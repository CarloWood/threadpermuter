#include "sys.h"
#include "ConditionVariable.h"
#include "debug.h"

namespace thread_permuter {

ConditionVariable::ConditionVariable() : m_waiting_threads{0}, m_was_notify_one{0}
{
  DoutEntering(dc::notice, "ConditionVariable::ConditionVariable() [" << (void*)this << "]");
}

void ConditionVariable::wait(std::unique_lock<Mutex>& lock)
{
  m_waiting_threads |= index2mask(Thread::current()->get_thi());
  DoutEntering(dc::notice|flush_cf, "ConditionVariable::wait() [" << (void*)this << "]; there are now " <<
      m_waiting_threads.count() << " threads waiting on " << (void*)this << " (" << m_waiting_threads << ")");
  lock.unlock();
  Thread::wait(this);
  lock.lock();
  if (m_was_notify_one)
  {
    ASSERT(m_was_notify_one == 1);
    --m_was_notify_one;
    Thread::woken(this);  // Recover from what Thread::notify_one(this) did.
  }
  m_waiting_threads &= ~index2mask(Thread::current()->get_thi());
  Dout(dc::notice|flush_cf, "Leaving ConditionVariable::wait; there are now " << m_waiting_threads.count() <<
      " threads waiting on " << (void*)this << " (" << m_waiting_threads << ")");
}

void ConditionVariable::notify_one() noexcept
{
  DoutEntering(dc::notice, "ConditionVariable::notify_one() [" << (void*)this << "]");
  if (m_waiting_threads.any())
  {
    Thread::notify_one(this);
    ++m_was_notify_one;
  }
}

void ConditionVariable::notify_all() noexcept
{
  DoutEntering(dc::notice, "ConditionVariable::notify_all() [" << (void*)this << "]");
  Thread::notify_all(this);
}

void ConditionVariable::clear_waiting_threads()
{
  DoutEntering(dc::notice, "ConditionVariable::clear_waiting_threads() [" << (void*)this << "]");
  m_waiting_threads.reset();
}

} // namespace thread_permuter
