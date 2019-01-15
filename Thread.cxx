#include "Thread.h"

namespace thread_permuter {

Thread::Thread(std::function<void()> test) : m_test(test)
{
}

void Thread::start()
{
}

bool Thread::step()
{
  m_test();
  return true;
}

} // namespace thread_permuter
