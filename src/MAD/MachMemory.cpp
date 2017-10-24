// Std
#include <cassert>
#include <vector>

// MAD
#include "MAD/MachMemory.hpp"
#include "MAD/MachMemoryRegion.hpp"
#include <MAD/Error.hpp>

using namespace mad;

bool MachMemory::Init(mach_port_t TaskPort) {
  this->Port = TaskPort;

  mach_msg_type_number_t InfoCount = TASK_VM_INFO_COUNT;
  task_vm_info_data_t Info;
  if (Error Err =
          task_info(Port, TASK_VM_INFO, (task_info_t)&Info, &InfoCount)) {
    Err.Log();
    return false;
  }

  PageSize = Info.page_size;

  return true;
}

void MachMemory::Fini() {
  this->Port = 0;
  this->PageSize = 0;
}

std::vector<MachMemoryRegion> MachMemory::GetRegions(mach_vm_address_t Address,
                                                     mach_vm_size_t Size) {
  std::vector<MachMemoryRegion> Regions;

  while (Size) {
    MachMemoryRegion Region(Port, Address);
    if (!Region.IsValid()) {
      Regions.push_back(std::move(Region));
      break;
    }

    mach_vm_address_t Covered = Region.GetFollowingAddress() - Address;
    if (Covered > Size) {
      Covered = Size;
    }

    Address += Covered;
    Size -= Covered;

    Regions.push_back(std::move(Region));
  }
  return Regions;
}

mach_vm_size_t
MachMemory::WriteToRegions(std::vector<MachMemoryRegion> &Regions,
                           mach_vm_address_t Address, vm_offset_t Data,
                           mach_msg_type_number_t Size) {
  // Before we write anything a proper protection level must be set on ALL the
  // regions at the same time.  If some region does not allow write permissions
  // we cannot continue. Protection level will be restored once the region
  // object destoyed.
  for (auto &Region : Regions) {
    if (!Region.SetProtection(VM_PROT_READ | VM_PROT_WRITE)) {
      return 0;
    }
  }

  mach_vm_size_t Written = 0;
  for (auto &Region : Regions) {
    assert(Size);
    mach_vm_address_t ToWrite = Region.GetFollowingAddress() - Address;
    if (ToWrite > Size) {
      ToWrite = Size;
    }

    if (Error Err = mach_vm_write(Port, Address, Data, ToWrite)) {
      // TODO: Specify which region failed
      Err.Log("At", HEX(Address), "writing", Size, "bytes");
      return Written;
    }

    Written += ToWrite;
    Address += ToWrite;
    Data += ToWrite;
    Size -= ToWrite;
  }

  return Written;
}

mach_vm_size_t MachMemory::Read(mach_vm_address_t Address, mach_vm_size_t size,
                                void *Data) {
  assert(Port);
  mach_vm_size_t Size;
  if (Error Err = mach_vm_read_overwrite(Port, Address, size,
                                         (mach_vm_address_t)Data, &Size)) {
    Err.Log("At", HEX(Address), "reading", size, "bytes");
    return 0;
  }
  return Size;
}

mach_vm_size_t MachMemory::Write(mach_vm_address_t Address, vm_offset_t Data,
                                 mach_msg_type_number_t Size) {
  assert(Port);

  std::vector<MachMemoryRegion> Regions = GetRegions(Address, Size);
  if (!Regions.back().IsValid()) {
    Error Err(MAD_ERROR_MEMORY);
    Err.Log("Cannot use a memory region pointed of ", Address, "-", Address + Size);
    return 0;
  }

  return WriteToRegions(Regions, Address, Data, Size);
}
