#ifndef MACHMEMORYREGION_HPP_9QWIUVX0
#define MACHMEMORYREGION_HPP_9QWIUVX0

// System
#include <mach/mach.h>
#include <mach/mach_vm.h>

// MAD
#include <MAD/Error.hpp>

namespace mad {
class MachMemoryRegion {
  Error Err;
  mach_port_t Port;
  mach_vm_address_t Address;
  mach_vm_size_t Size;
  natural_t Depth;
  vm_prot_t CurrentProtection = VM_PROT_NONE;
  vm_prot_t MaximumProtection = VM_PROT_NONE;
  vm_region_submap_short_info_data_64_t Data;

public:
  MachMemoryRegion(mach_port_t Port, vm_address_t Address);
  MachMemoryRegion(const MachMemoryRegion &) = delete;
  MachMemoryRegion &operator=(const MachMemoryRegion &) = delete;
  MachMemoryRegion(MachMemoryRegion &&) = default;
  MachMemoryRegion &operator=(MachMemoryRegion &&) = default;
  ~MachMemoryRegion();

  bool IsValid() { return Err.Success(); }
  auto GetError() { return Err; }

  auto GetAddress() { return Address; }
  auto GetFollowingAddress() { return Address + Size; }
  auto GetSize() { return Size; }
  auto GetCurrentProtection() { return CurrentProtection; }
  auto GetMaximumProtection() { return MaximumProtection; }

  bool SetProtection(vm_prot_t Protection);
  bool RestoreProtection();
};
} // namespace mad

#endif /* end of include guard: MACHMEMORYREGION_HPP_9QWIUVX0 */
