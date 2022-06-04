#pragma once

#include "debug.h"
#include "utils/Vector.h"
#include "utils/BitSet.h"
#include <functional>
#include <thread>
#include <condition_variable>

#if defined(CWDEBUG) && !defined(DOXYGEN)
NAMESPACE_DEBUG_CHANNELS_START
extern channel_ct permutation;
NAMESPACE_DEBUG_CHANNELS_END
#endif

// VectorIndex category.
namespace vector_index_category {

// An index that specifies the test function (and corresponding thread).
// Initially defined by the first parameter passed to the constructor of ThreadPermuter.
struct thread_index;

} // namespace vector_index_category

// A type that can be used as index into ThreadPermuter::tests_type and ThreadPermuter::threads_type.
class ThreadIndex : public utils::VectorIndex<vector_index_category::thread_index>
{
  using utils::VectorIndex<vector_index_category::thread_index>::VectorIndex;

 public:
  // Allow bitset::Index to be used where ThreadIndex is required.
  ThreadIndex(utils::bitset::Index index) : utils::VectorIndex<vector_index_category::thread_index>(index()) { }
};

namespace thread_permuter {

using mask_type = uint32_t;
using threads_set_type = utils::BitSet<mask_type>;

// Allow conversion from ThreadIndex to threads_set_type.
inline threads_set_type index2mask(ThreadIndex thi)
{
  return threads_set_type(mask_type{1} << thi.get_value());
}

} // namespace thread_permuter

class PermutationFailure final : public std::runtime_error
{
  char const* m_file;
  int m_line;

 public:
  PermutationFailure() : std::runtime_error("<no error>"), m_file("<no file>"), m_line(-1) { }
  PermutationFailure(char const* msg, char const* file, int line) : std::runtime_error(msg), m_file(file), m_line(line) { }

  std::string message() const
  {
    std::string msg("\"");
    msg += std::runtime_error::what();
    msg += "\" in ";
    msg += m_file;
    msg += ':';
    msg += std::to_string(m_line);
    return msg;
  }
};

namespace thread_permuter {

enum state_type
{
  yielding,                     // Just yielding.
  blocking,                     // This thread should not be run anymore until at least one other thread did run.
  blocking_with_progress,       // This thread should not be run anymore until at least one other thread did run, all other threads are allowed to run again.
  waiting,                      // This thread should not be run anymore until another thread calls notify_one or notify_all on the associated condition variable.
  woken,                        // Like yielding, but when called after a condition variable had notify_one called on it, then block all other waiting threads again.
  notify_one,                   // This thread just called notify_one.
  notify_all,                   // This thread just called notify_all.
  failed,                       // This thread encountered an error condition and threw an exception.
  finished                      // Returned from m_test().
};

#ifdef CWDEBUG
std::string to_string(state_type state);
inline std::ostream& operator<<(std::ostream& os, state_type state)
{
  return os << to_string(state);
}
#endif

class ConditionVariable;

class Thread
{
 public:
  Thread(std::pair<std::function<void()>, ThreadIndex> const& args);
  ThreadIndex get_thi() const { return m_thi; }

  void start(char thread_name, bool debug_off); // Start the thread and prepare calling step().
  void run(bool debug_off);             // Entry point of m_thread.
  state_type step(bool& debug_on);      // Wake up the thread and let it run till the next check point (or finish).
                                        // Returns true when m_test() returned.
  void pause(state_type state);         // Pause the thread and wake up the main thread again.
  void stop();                          // Called when all permutation have been run.
  void made_progress() { m_progress = true; }
  ConditionVariable* condition_variable() const { return m_condition_variable; }

  char get_name() const { return m_thread_name; }
  PermutationFailure failure() const { return m_failure; }

 private:
  ThreadIndex m_thi;                    // The index of this thread.
  std::function<void()> m_test;         // Thread entry point. The first time step() is called
                                        // after start(), this function will be called.
  std::thread m_thread;                 // The actual thread.
  state_type m_state;
  bool m_last_permutation;              // True after all permutation have been run.
  ConditionVariable* m_condition_variable; // Valid when pause is called with waiting, notify_one or notify_all.

  std::condition_variable m_paused_condition;
  std::mutex m_paused_mutex;
  bool m_paused;                        // True when the thread is waiting.
  bool m_debug_on;                      // Set to true when debug output must be turned on in this thread.
  bool m_progress;                      // Set to true when TPP is used; causes the next TPB to call pause(blocking_with_progress);
  PermutationFailure m_failure;         // Error of last exception thrown.

  char m_thread_name;                   // Used for debugging output; set by start().

  static thread_local Thread* tl_self;  // A thread_local pointer to self.

 public:
  static void yield() { tl_self->pause(yielding); }
  static void blocked() { tl_self->pause(blocking); }
  static void wait(ConditionVariable* condition_variable) { tl_self->m_condition_variable = condition_variable; tl_self->pause(waiting); }
  static void woken(ConditionVariable* condition_variable) { tl_self->m_condition_variable = condition_variable; tl_self->pause(thread_permuter::woken); }
  static void notify_one(ConditionVariable* condition_variable) { tl_self->m_condition_variable = condition_variable; tl_self->pause(thread_permuter::notify_one); }
  static void notify_all(ConditionVariable* condition_variable) { tl_self->m_condition_variable = condition_variable; tl_self->pause(thread_permuter::notify_all); }
  static void progress() { tl_self->made_progress(); }
  static void fail(PermutationFailure const& error) { tl_self->m_failure = error; tl_self->pause(failed); }
  static char name() { return tl_self->get_name(); }
  static Thread* current() { return tl_self; }
};

// Use this instead of std::mutex.
class Mutex
{
 private:
  std::mutex m_mutex;

 public:
  void lock()
  {
    DoutEntering(dc::permutation|continued_cf, "Mutex::lock() [" << (void*)this << "]... ");
    while (!m_mutex.try_lock())
    {
      Dout(dc::permutation, "Blocked on mutex [" << (void*)this << "]");
      Thread::blocked();
    }
    Dout(dc::finish, "successfully locked [" << (void*)this << "]");
  }

  bool try_lock()
  {
    DoutEntering(dc::permutation|continued_cf, "Mutex::try_lock() [" << (void*)this << "]... ");
    bool locked = m_mutex.try_lock();
    Dout(dc::finish, (locked ? "locked" : "failed"));
    return locked;
  }

  void unlock()
  {
    DoutEntering(dc::permutation, "Mutex::unlock() [" << (void*)this << "]");
    m_mutex.unlock();
  }

  auto native_handle()
  {
    return m_mutex.native_handle();
  }
};

} // namespace thread_permuter

// Use this to make the thread yield and either continue with a different thread or with the same thread again.
#define TPY do { Dout(dc::permutation, "TPY at " << __FILE__ << ":" << __LINE__); thread_permuter::Thread::yield(); } while(0)
// Use this to make the thread yield and force the run of another thread before running this thread again.
#define TPB do { Dout(dc::permutation, "TPB at " << __FILE__ << ":" << __LINE__); thread_permuter::Thread::blocked(); } while(0)
// Use this just before a TPB if the thread made any progress, so that it is ok to run other, previously blocking threads.
#define TPP do { Dout(dc::permutation, "TPP at " << __FILE__ << ":" << __LINE__); thread_permuter::Thread::progress(); } while(0)
