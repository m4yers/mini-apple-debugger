#include <cassert>
#include <mach/mach.h>

#include "MAD/Debug.hpp"
#include "MAD/MachTask.hpp"

using namespace mad;

bool MachTask::Attach(pid_t pid) {
  assert(!Port);

  PID = pid;

  if (auto kern = task_for_pid(mach_task_self(), PID, &Port)) {
    PRINT_KERN_ERROR(kern);
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
  if (auto kern = task_suspend(Port)) {
    PRINT_KERN_ERROR(kern);
    return false;
  }
  Suspended = true;
  return true;
}

bool MachTask::Resume() {
  assert(Suspended);
  assert(Port);
  if (auto kern = task_resume(Port)) {
    PRINT_KERN_ERROR(kern);
    return false;
  }
  Suspended = false;
  return true;
}

Optional<std::vector<MachThread>> MachTask::GetThreads(bool DoSuspend) {
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

  if (auto kern = task_threads(Port, &thread_list, &thread_count)) {
    PRINT_KERN_ERROR(kern);
    return {};
  }

  std::vector<MachThread> result;
  for (mach_msg_type_number_t i = 0; i < thread_count; ++i) {
    result.emplace_back(Port, thread_list[i]);
  }

  if (WakeAfter) {
    Resume();
  }

  return result;
}
