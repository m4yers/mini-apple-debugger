#include <cassert>
#include <mach/mach.h>

#include "MAD/Error.hpp"
#include "MAD/MachTask.hpp"

using namespace mad;

bool MachTask::Attach(pid_t pid) {
  assert(!Port);

  PID = pid;

  if (Error Err = task_for_pid(mach_task_self(), PID, &Port)) {
    Err.Log("Could not get Task Port for PID", PID);
    return false;
  }

  return Memory.Init(Port);
}

bool MachTask::Detach() {
  assert(Port);
  PID = {};
  Memory.Fini();
  return true;
}

bool MachTask::Suspend() {
  assert(!Suspended);
  assert(Port);
  if (Error Err = task_suspend(Port)) {
    Err.Log("Could not suspend Task ", PID);
    return false;
  }
  Suspended = true;
  return true;
}

bool MachTask::Resume() {
  assert(Suspended);
  assert(Port);
  if (Error Err = task_resume(Port)) {
    Err.Log("Could not resume Task ", PID);
    return false;
  }
  Suspended = false;
  return true;
}

// FIXME: Not cool...
std::vector<MachThread> MachTask::GetThreads(bool DoSuspend) {
  assert(Port);

  thread_act_port_array_t thread_list;
  mach_msg_type_number_t thread_count;

  bool WakeAfter = false;
  if (!Suspended && DoSuspend) {
    if (!Suspend()) {
      return {};
    }
    WakeAfter = true;
  }

  if (Error Err = task_threads(Port, &thread_list, &thread_count)) {
    Err.Log("Could not fetch threads for", PID);
    return {};
  }

  std::vector<MachThread> result;
  for (mach_msg_type_number_t i = 0; i < thread_count; ++i) {
    result.emplace_back(thread_list[i]);
  }

  if (WakeAfter) {
    Resume();
  }

  return result;
}
