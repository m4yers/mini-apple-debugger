// Std
#include <cstring>

// MAD
#include "MAD/Debug.hpp"
#include "MAD/MachMemoryRegion.hpp"

using namespace mad;

MachMemoryRegion::MachMemoryRegion(mach_port_t Port,
                                   vm_address_t RequestedAddress)
    : Port(Port), Address(RequestedAddress) {

  mach_msg_type_number_t InfoSize = VM_REGION_SUBMAP_SHORT_INFO_COUNT_64;
  if (auto kern = mach_vm_region_recurse(Port, &Address, &Size, &Depth,
                                         (vm_region_recurse_info_64_t)&Data,
                                         &InfoSize)) {
    PRINT_ERROR("vm_region_recurse");
    Valid = false;
  }

  // It is possible for vm_region_recurse return a region that does not contain
  // requested address. This happens if the requested address points to
  // unmapped part of memory.
  if (RequestedAddress < Address || RequestedAddress >= (Address + Size)) {
    PRINT_ERROR("Requested address is part of any region");
    Valid = false;
  } else {
    CurrentProtection = Data.protection;
    MaximumProtection = Data.max_protection;
    Depth = 1024;
    Valid = true;
  }
}

MachMemoryRegion::~MachMemoryRegion() {
  RestoreProtection();
  std::memset(&Data, 0, sizeof(Data));
  Port = -1;
  Address = -1;
  Size = -1;
  Depth = -1;
  CurrentProtection = VM_PROT_NONE;
  MaximumProtection = VM_PROT_NONE;
}

bool MachMemoryRegion::SetProtection(vm_prot_t Protection) {
  if (!IsValid()) {
    return false;
  }

  if (auto kern = vm_protect(Port, Address, Size, false, Protection)) {
    PRINT_KERN_ERROR_V(kern, " at ", HEX(Address), " setting protection to ",
                       HEX(Protection));
    Valid = false;
    return false;
  }

  CurrentProtection = Protection;

  return true;
}

bool MachMemoryRegion::RestoreProtection() {
  if (!IsValid()) {
    return false;
  }

  if (Data.protection == CurrentProtection) {
    return true;
  }

  if (!SetProtection(Data.protection)) {
    Valid = false;
    return false;
  }

  return true;
}
