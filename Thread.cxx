#include "sys.h"
#include "Thread.h"
#include "debug.h"
#include <mutex>

#ifndef CWDEBUG
#error "You really want to compile this with libcwd enabled (cmake: -DEnableDebug:BOOL=ON)"
#endif

namespace thread_permuter {

Thread::Thread(std::function<void()> test) :
  m_test(test), m_state(yielding),
  m_last_permutation(false), m_paused(false), m_debug_on(false),
  m_thread_name('?')
{
}

void Thread::start(char thread_name, bool debug_off)
{
  m_thread_name = thread_name;
  std::unique_lock<std::mutex> lock(m_paused_mutex);
  // Start thread.
  m_thread = std::thread([this, debug_off](){ Thread::run(debug_off); });
  // Wait until the thread is paused.
  m_paused_condition.wait(lock, [this]{ return m_paused; });
}

void Thread::run(bool debug_off)
{
  if (debug_off)
    Debug(libcw_do.off());
  Debug(NAMESPACE_DEBUG::init_thread(std::string("thread") + m_thread_name));
  tl_self = this;                       // Allow a checkpoint to find this object back.
  pause(yielding);                      // Wait until we may enter m_test() for the first time.
  do
  {
    try
    {
      m_test();                         // Call the test function.
    }
    catch (PermutationFailure const& error)
    {
#ifdef CWDEBUG
      libcwd::debug_ct::OnOffState state;
      Debug(libcw_do.force_on(state));
#endif
      fail(error);
      continue;
    }
    pause(finished);                    // Wait till we may continue with the next permutation.
  }
  while (!m_last_permutation);          // if any.
#ifdef CWDEBUG
  libcwd::debug_ct::OnOffState state;
  Debug(libcw_do.force_on(state));
#endif
  Dout(dc::notice|flush_cf, "Leaving Thread::run()");
  Debug(libcw_do.restore(state));
}

void Thread::pause(state_type state)
{
  m_state = state;
  std::unique_lock<std::mutex> lock(m_paused_mutex);
  m_paused = true;
  m_paused_condition.notify_one();
  m_paused_condition.wait(lock, [this]{ return !m_paused; });
  if (m_debug_on)
  {
    Debug(libcw_do.on());
    m_debug_on = false;
  }
}

state_type Thread::step(bool& debug_on)
{
  std::unique_lock<std::mutex> lock(m_paused_mutex);
  m_paused = false;
  if (debug_on)
  {
    m_debug_on = true;
    debug_on = false;
  }
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

#if defined(CWDEBUG) && !defined(DOXYGEN)
NAMESPACE_DEBUG_CHANNELS_START
channel_ct permutation("PERMUTATION");
NAMESPACE_DEBUG_CHANNELS_END
#endif
