#pragma once

#include "Permutation.h"
#include <mutex>

namespace thread_permuter {

class ConditionVariable
{
 private:
  threads_set_type m_waiting_threads;
  int m_was_notify_one;

 public:
  ConditionVariable();

  void wait(std::unique_lock<Mutex>& lock);
  void notify_one() noexcept;
  void notify_all() noexcept;
  void clear_waiting_threads();

  threads_set_type waiting_threads() const { return m_waiting_threads; }

  template<typename Predicate>
  void wait(std::unique_lock<Mutex>& lock, Predicate p)
  {
    while (!p())
      wait(lock);
  }
};

} // namespace thread_permuter
