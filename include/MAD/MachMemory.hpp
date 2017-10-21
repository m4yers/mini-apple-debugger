#ifndef MACHMEMORY_HPP_FT3BKH4I
#define MACHMEMORY_HPP_FT3BKH4I

// System
#include <mach/mach.h>
#include <unistd.h>

namespace mad {
class MachTask;
class MachMemory {
  friend MachTask;

private:
  mach_port_t Port;
  vm_size_t PageSize;

private:
  bool Init(mach_port_t Port);
  void Fini();

public:
  MachMemory() : Port(0), PageSize(0) {}

  MachMemory(const MachMemory &) = delete;
  MachMemory operator=(const MachMemory &) = delete;

  vm_size_t GetPageSize() { return PageSize; }

  bool SetMaxiumuProtection(vm_address_t address, vm_size_t size,
                            vm_prot_t level);
  bool SetCurrentProtection(vm_address_t address, vm_size_t size,
                            vm_prot_t level);

  vm_size_t Read(vm_address_t address, vm_size_t size, void *data);
  vm_size_t Write(vm_address_t address, vm_offset_t data,
                  mach_msg_type_number_t count);
};
} // namespace mad

#endif /* end of include guard: MACHMEMORY_HPP_FT3BKH4I */
