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
  return true;
}

bool MachTask::Detach() {
  assert(Port);
  PID = {};
  return true;
}

vm_size_t MachTask::GetPageSize() {
  vm_size_t size;
  if (auto kern = host_page_size(mach_host_self(), &size)) {
    PRINT_KERN_ERROR(kern);
    return 0;
  }
  return size;
}

vm_size_t MachTask::ReadMemory(vm_address_t address, vm_size_t size,
                               void *data) {
  assert(Port);
  vm_size_t count;
  if (auto kern =
          vm_read_overwrite(Port, address, size, (vm_address_t)data, &count)) {
    PRINT_KERN_ERROR_V(kern, " at ", HEX(address));
    return 0;
  }
  return count;
}

vm_size_t MachTask::WriteMemory(vm_address_t address, vm_offset_t data,
                                mach_msg_type_number_t count) {
  assert(Port);
  if (auto kern = vm_write(Port, address, data, count)) {
    PRINT_KERN_ERROR_V(kern, " at ", HEX(address));
    return 0;
  }
  return count;
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
  result.reserve(thread_count);
  for (mach_msg_type_number_t i = 0; i < thread_count; ++i) {
    result.emplace_back(Port, thread_list[i]);
  }

  if (WakeAfter) {
    Resume();
  }

  return result;
}
