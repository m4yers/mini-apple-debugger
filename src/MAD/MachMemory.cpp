#include <cassert>

#include "MAD/MachMemory.hpp"
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

bool MachMemory::SetMaxiumuProtection(vm_address_t address, vm_size_t size,
                                      vm_prot_t level) {
  assert(Port);
  if (auto kern = vm_protect(Port, address, size, true, level)) {
    PRINT_KERN_ERROR_V(kern, " at ", HEX(address), " setting protection to ",
                       HEX(level));
    return false;
  }

  return true;
}

bool MachMemory::SetCurrentProtection(vm_address_t address, vm_size_t size,
                                      vm_prot_t level) {
  assert(Port);
  if (auto kern = vm_protect(Port, address, size, false, level)) {
    PRINT_KERN_ERROR_V(kern, " at ", HEX(address), " setting protection to ",
                       HEX(level));
    return false;
  }

  return true;
}

vm_size_t MachMemory::Read(vm_address_t address, vm_size_t size, void *data) {
  assert(Port);
  vm_size_t count;
  if (auto kern =
          vm_read_overwrite(Port, address, size, (vm_address_t)data, &count)) {
    PRINT_KERN_ERROR_V(kern, " at ", HEX(address), " reading ", size, " bytes");
    return 0;
  }
  return count;
}

vm_size_t MachMemory::Write(vm_address_t address, vm_offset_t data,
                            mach_msg_type_number_t count) {
  assert(Port);
  if (auto kern = vm_write(Port, address, data, count)) {
    PRINT_KERN_ERROR_V(kern, " at ", HEX(address), " writing ", count,
                       " bytes");
    return 0;
  }
  return count;
}
