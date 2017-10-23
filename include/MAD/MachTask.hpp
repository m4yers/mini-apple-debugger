#ifndef MACHTASK_HPP_3NTPLSFZ
#define MACHTASK_HPP_3NTPLSFZ

// System
#include <mach/mach.h>
#include <unistd.h>

// Std
#include <cstring>
#include <experimental/optional>
#include <vector>

// MAD
#include "MAD/Debug.hpp"
#include "MAD/MachThread.hpp"
#include <MAD/MachMemory.hpp>

template <typename T> using Optional = std::experimental::optional<T>;

namespace mad {
class MachTask {
  pid_t PID;
  mach_port_t Port;
  MachMemory Memory;
  bool Suspended;

public:
  MachTask() : PID(0), Port(0), Suspended(false){};
  MachTask(const MachTask &) = delete;
  MachTask operator=(const MachTask &) = delete;

  bool Attach(pid_t PID);
  bool Detach();

  bool IsAttached() { return Port; }

  mach_port_t GetPort() { return Port; }

  bool Suspend();
  bool Resume();

  auto &GetMemory() { return Memory; }
  std::vector<MachThread> GetThreads(bool Suspend = false);
};
} // namespace mad

#endif /* end of include guard: MACHTASK_HPP_3NTPLSFZ */
