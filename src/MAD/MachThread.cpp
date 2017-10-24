#include "MAD/MachThread.hpp"
#include "MAD/Error.hpp"

using namespace mad;

bool MachThread::GetThreadState() {

  if (Error Err = thread_get_state(Id, x86_THREAD_STATE, (thread_state_t)&thread_state,
                       &thread_state_count)) {
    Err.Log("Could not get thread state");
    return false;
  }

  return true;
}

bool MachThread::SetThreadState() {
  if (Error Err = thread_set_state(Id, x86_THREAD_STATE, (thread_state_t)&thread_state,
                       thread_state_count)) {
    Err.Log("Could not set thread state");
    return false;
  }

  return true;
}

bool MachThread::GetStates() { return GetThreadState(); }

bool MachThread::SetStates() { return SetThreadState(); }
