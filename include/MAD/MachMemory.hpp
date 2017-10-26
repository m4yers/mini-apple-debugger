#ifndef MACHMEMORY_HPP_FT3BKH4I
#define MACHMEMORY_HPP_FT3BKH4I

// System
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <unistd.h>

// Std
#include <vector>

// MAD
#include <MAD/MachMemoryRegion.hpp>

namespace mad {
class MachTask;
class MachMemory {
  friend MachTask;

private:
  mach_port_t Port;
  mach_vm_size_t PageSize;

private:
  bool Init(mach_port_t Port);
  void Fini();

  // Before we do anything with task memory we locate all regions that cover
  // the specified address and size, verifying that they are contigues and do
  // not contain gaps. If there is a gap the last vector element will contain
  // an invalid region.
  std::vector<MachMemoryRegion> GetRegions(mach_vm_address_t Address,
                                           mach_vm_size_t Size);

  // Write a byte data array into specified region vector starting from the
  // Address. This function assumes the Address is contained in the first
  // Region of the vector.
  mach_vm_size_t WriteToRegions(std::vector<MachMemoryRegion> &Regions,
                                mach_vm_address_t Address, vm_offset_t Data,
                                mach_msg_type_number_t Size);
  mach_vm_size_t ReadFromRegions(std::vector<MachMemoryRegion> &Regions,
                                 mach_vm_address_t Address, vm_offset_t Data,
                                 mach_msg_type_number_t Size);

public:
  MachMemory() : Port(0), PageSize(0) {}

  MachMemory(const MachMemory &) = delete;
  MachMemory operator=(const MachMemory &) = delete;

  mach_vm_size_t GetPageSize() { return PageSize; }

  mach_vm_size_t Read(mach_vm_address_t address, mach_vm_size_t size,
                      void *data);
  mach_vm_size_t Write(mach_vm_address_t address, vm_offset_t data,
                       mach_msg_type_number_t count);
};
} // namespace mad

#endif /* end of include guard: MACHMEMORY_HPP_FT3BKH4I */
