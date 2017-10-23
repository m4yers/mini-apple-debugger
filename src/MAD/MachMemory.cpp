// Std
#include <cassert>
#include <vector>

// MAD
#include "MAD/MachMemory.hpp"
#include "MAD/MachMemoryRegion.hpp"
#include <MAD/Debug.hpp>

using namespace mad;

bool MachMemory::Init(mach_port_t Port) {
  this->Port = Port;

  mach_msg_type_number_t InfoCount = TASK_VM_INFO_COUNT;
  task_vm_info_data_t Info;
  if (auto kern =
          task_info(Port, TASK_VM_INFO, (task_info_t)&Info, &InfoCount)) {
    PRINT_KERN_ERROR(kern);
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

    if (auto kern = mach_vm_write(Port, Address, Data, ToWrite)) {
      // TODO: Specify which region failed
      PRINT_KERN_ERROR_V(kern, " at ", HEX(Address), " writing ", Size,
                         " bytes");
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
  if (auto kern = mach_vm_read_overwrite(Port, Address, size,
                                    (mach_vm_address_t)Data, &Size)) {
    PRINT_KERN_ERROR_V(kern, " at ", HEX(Address), " reading ", size, " bytes");
    return 0;
  }
  return Size;
}

mach_vm_size_t MachMemory::Write(mach_vm_address_t Address, vm_offset_t Data,
                                 mach_msg_type_number_t Size) {
  assert(Port);

  std::vector<MachMemoryRegion> Regions = GetRegions(Address, Size);
  if (!Regions.back().IsValid()) {
    // TODO: Once proper erroring is in place pls qualify what the error is
    PRINT_ERROR("Cannot write to ", HEX(Address), " ", Size, " bytes");
    return 0;
  }

  return WriteToRegions(Regions, Address, Data, Size);
}
