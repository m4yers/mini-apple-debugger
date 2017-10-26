// Std
#include <cstring>

// MAD
#include "MAD/Error.hpp"
#include "MAD/MachMemoryRegion.hpp"

using namespace mad;

MachMemoryRegion::MachMemoryRegion(mach_port_t Port,
                                   vm_address_t RequestedAddress)
    : Port(Port), Address(RequestedAddress) {

  mach_msg_type_number_t InfoSize = VM_REGION_SUBMAP_SHORT_INFO_COUNT_64;
  Err = mach_vm_region_recurse(Port, &Address, &Size, &Depth,
                               (vm_region_recurse_info_64_t)&Data, &InfoSize);
  if (Err) {
    Err.Log("Could not get region at", HEX(RequestedAddress), "for PORT", Port);
  } else {
    // It is possible for vm_region_recurse return a region that does not
    // contain requested address. This happens if the requested address points
    // to unmapped part of memory.
    if (RequestedAddress < Address || RequestedAddress >= (Address + Size)) {
      Err = Error(MAD_ERROR_MEMORY);
      Err.Log("Requested address ", HEX(Address),
              "is not part of any region of PORT", Port);
    } else {
      CurrentProtection = Data.protection;
      MaximumProtection = Data.max_protection;
      Depth = 1024;
    }
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

  Err = vm_protect(Port, Address, Size, false, Protection);
  if (Err) {
    Err.Log("At", HEX(Address), "setting protection to", HEX(Protection));
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

  return SetProtection(Data.protection);
}
