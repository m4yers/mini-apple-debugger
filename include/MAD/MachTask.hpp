#ifndef MACHTASK_HPP_3NTPLSFZ
#define MACHTASK_HPP_3NTPLSFZ

// System
#include <mach/mach.h>
#include <unistd.h>

// Std
#include <cstring>
#include <experimental/optional>
#include <vector>

// MAD
#include "MAD/Debug.hpp"
#include "MAD/MachThread.hpp"

template <typename T> using Optional = std::experimental::optional<T>;

namespace mad {
class MachTask {
  pid_t PID;
  mach_port_t Port;
  bool Suspended;

public:
  MachTask() : PID(0), Port(0), Suspended(false){};

  bool Attach(pid_t PID);
  bool Detach();

  bool IsAttached() { return Port; }

  mach_port_t GetPort() { return Port; }

  vm_size_t GetPageSize();
  vm_size_t ReadMemory(vm_address_t address, vm_size_t size, void *data);
  vm_size_t WriteMemory(vm_address_t address, vm_offset_t data,
                        mach_msg_type_number_t count);

  bool Suspend();
  bool Resume();

  Optional<std::vector<MachThread>> GetThreads(bool Suspend = false);
};
} // namespace mad

#endif /* end of include guard: MACHTASK_HPP_3NTPLSFZ */
