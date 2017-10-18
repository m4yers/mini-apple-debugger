#include "MAD/MachThread.hpp"
#include "MAD/Debug.hpp"

using namespace mad;

bool MachThread::GetThreadState() {

  if (auto kern = thread_get_state(id, x86_THREAD_STATE, (thread_state_t)&thread_state,
                       &thread_state_count)) {
    PRINT_KERN_ERROR(kern);
    return false;
  }

  return true;
}

bool MachThread::SetThreadState() {
  if (auto kern = thread_set_state(id, x86_THREAD_STATE, (thread_state_t)&thread_state,
                       thread_state_count)) {
    PRINT_KERN_ERROR(kern);
    return false;
  }

  return true;
}

bool MachThread::GetStates() { return GetThreadState(); }

bool MachThread::SetStates() { return SetThreadState(); }
