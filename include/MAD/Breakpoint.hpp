#ifndef BREAKPOINT_HPP_GMRO4YVQ
#define BREAKPOINT_HPP_GMRO4YVQ

#include <unistd.h>

#include "MAD/MachTask.hpp"

namespace mad {
class Breakpoint {
  MachTask &Task;
  uintptr_t Address;
  uintptr_t Original;

public:
  Breakpoint(MachTask &task, uintptr_t address)
      : Task(task), Address(address) {}

  bool IsEnabled();

  bool Enable();
  bool Disable();
};
} // namespace mad

#endif /* end of include guard: BREAKPOINT_HPP_GMRO4YVQ */
