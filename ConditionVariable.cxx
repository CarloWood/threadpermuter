#include "sys.h"
#include "ConditionVariable.h"
#include "debug.h"

namespace thread_permuter {

ConditionVariable::ConditionVariable() : m_waiting_threads{0}
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
  Thread::woken(this);  // In case that we were woken up by a notify_one, this is used to recover from what that did.
  m_waiting_threads &= ~index2mask(Thread::current()->get_thi());
  Dout(dc::notice|flush_cf, "Leaving ConditionVariable::wait; there are now " << m_waiting_threads.count() <<
      " threads waiting on " << (void*)this << " (" << m_waiting_threads << ")");
}

void ConditionVariable::notify_one() noexcept
{
  DoutEntering(dc::notice, "ConditionVariable::notify_one() [" << (void*)this << "]");
  Thread::notify_one(this);
}

void ConditionVariable::notify_all() noexcept
{
  DoutEntering(dc::notice, "ConditionVariable::notify_all() [" << (void*)this << "]");
  Thread::notify_all(this);
}

} // namespace thread_permuter
