#include "sys.h"
#include "Thread.h"
#include "debug.h"
#include <mutex>

namespace thread_permuter {

Thread::Thread(std::function<void()> test) :
  m_test(test), m_state(yielding),
  m_last_permutation(false), m_paused(false)
{
}

void Thread::start()
{
  std::unique_lock<std::mutex> lock(m_paused_mutex);
  // Start thread.
  m_thread = std::thread(&Thread::run, this);
  // Wait until the thread is paused.
  m_paused_condition.wait(lock, [this]{ return m_paused; });
}

void Thread::run()
{
  Debug(NAMESPACE_DEBUG::init_thread());
  tl_self = this;                       // Allow a checkpoint to find this object back.
  pause(yielding);                      // Wait until we may enter m_test() for the first time.
  do
  {
    m_test();                           // Call the test function.
    pause(finished);                    // Wait till we may continue with the next permutation.
  }
  while (!m_last_permutation);          // if any.
  Dout(dc::notice|flush_cf, "Leaving Thread::run()");
}

void Thread::pause(state_type state)
{
  m_state = state;
  std::unique_lock<std::mutex> lock(m_paused_mutex);
  m_paused = true;
  m_paused_condition.notify_one();
  m_paused_condition.wait(lock, [this]{ return !m_paused; });
}

state_type Thread::step()
{
  std::unique_lock<std::mutex> lock(m_paused_mutex);
  m_paused = false;
  // Wake up the thread.
  m_paused_condition.notify_one();
  // Wait until the thread is paused again.
  m_paused_condition.wait(lock, [this]{ return m_paused; });
  return m_state;
}

void Thread::stop()
{
  m_last_permutation = true;
  {
    std::unique_lock<std::mutex> lock(m_paused_mutex);
    m_paused = false;
    // Wake up the thread and let it exit.
    m_paused_condition.notify_one();
  }
  // Join with it.
  Dout(dc::notice|flush_cf, "Joining with thread " << std::hex << m_thread.get_id());
  m_thread.join();
}

//static
thread_local Thread* Thread::tl_self;

} // namespace thread_permuter
