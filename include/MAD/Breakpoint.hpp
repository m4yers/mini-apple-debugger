#ifndef BREAKPOINT_HPP_GMRO4YVQ
#define BREAKPOINT_HPP_GMRO4YVQ

// System
#include <mach/mach.h>
#include <unistd.h>

// MAD
#include "MAD/Error.hpp"
#include "MAD/MachMemory.hpp"

#define BREAKPOINT_INSTRUCTION 0xCC
#define BREAKPOINT_MASK ~0xFF

namespace mad {
class Breakpoint {
  MachMemory &Memory;
  uint64_t Address;
  uintptr_t Original;

public:
  Breakpoint(MachMemory &Memory, vm_address_t Address)
      : Memory(Memory), Address(Address) {}

  bool IsEnabled();
  auto GetAddress() { return Address; }

  bool Enable() {
    if (Memory.Read(Address, sizeof(Original), &Original) != sizeof(Original)) {
      Error Err(MAD_ERROR_BREAKPOINT);
      Err.Log("Could not set breakpoint at", HEX(Address));
      return false;
    }

    decltype(Original) Modified =
        (Original & BREAKPOINT_MASK) | BREAKPOINT_INSTRUCTION;

    if (Memory.Write(Address, (vm_offset_t)&Modified, sizeof(Modified)) !=
        sizeof(Original)) {
      Error Err(MAD_ERROR_BREAKPOINT);
      Err.Log("Could not set breakpoint at", HEX(Address));
      return false;
    }

    return true;
  }

  bool Disable() {
    if (Memory.Write(Address, (vm_offset_t)&Original, sizeof(Original)) !=
        sizeof(Original)) {
      Error Err(MAD_ERROR_BREAKPOINT);
      Err.Log("Could not remove breakpoint at", HEX(Address));
      return false;
    }

    return true;
  }
};
} // namespace mad

#endif /* end of include guard: BREAKPOINT_HPP_GMRO4YVQ */
