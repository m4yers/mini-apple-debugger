#ifndef BREAKPOINTS_HPP_SKXFYM5R
#define BREAKPOINTS_HPP_SKXFYM5R

// System
#include <unistd.h>

// Std
#include <experimental/any> // Rewrite using variant later...
#include <functional>
#include <map>
#include <memory>

// MAD
#include "MAD/MachMemory.hpp"
#include "MAD/MachProcess.hpp"

namespace mad {

enum class BreakpointCallbackReturn {
  REMOVE,
  REMOVE_CONTINUE,
  STEP_OVER,
  STEP_OVER_CONTINUE
};

using BreakpointByAddress_t =
    std::function<BreakpointCallbackReturn(vm_address_t)>;
using BreakpointBySymbolName_t =
    std::function<BreakpointCallbackReturn(std::string)>;

enum class BreakpointType { Address, Symbol };
class BreakpointSeedAddress {
public:
  BreakpointByAddress_t Callback;
  vm_address_t Address;
};
class BreakpointSeedSymbol {
public:
  BreakpointBySymbolName_t Callback;
  // FIXME: Not cool, do something here...
  std::shared_ptr<MachOParser64::MachOSymbolTableEntry> Symbol;
};

class Breakpoint {
public:
  BreakpointType Type;
  std::experimental::any Seed;
  MachMemory &Memory;
  uint64_t Address;
  uintptr_t Original;

public:
  template <typename S>
  Breakpoint(BreakpointType Type, S Seed, MachMemory &Memory,
             vm_address_t Address)
      : Type(Type), Seed(Seed), Memory(Memory), Address(Address) {}

  bool IsEnabled();
  auto GetAddress() { return Address; }

  bool Enable();
  bool Disable();
};

class Breakpoints {
  MachProcess &Process;
  std::map<uintptr_t, std::shared_ptr<Breakpoint>> BreakpointsByAddress;
  std::map<uintptr_t, std::vector<std::shared_ptr<Breakpoint>>>
      BreakpointsByPage;

public:
  Breakpoints(MachProcess &Process) : Process(Process) {}

  void AddBreakpointByAddress(vm_address_t Address, BreakpointByAddress_t);
  void AddBreakpointBySymbolName(std::string SymbolName,
                                 BreakpointBySymbolName_t);

  void PreRun();
  void PostRun();
  bool CheckBreakpoints();

private:
  void AddBreakpoint(std::shared_ptr<Breakpoint>);
  void HandleDebuggerNotification();
  void HandleRemove(std::shared_ptr<Breakpoint> &);
  void HandleStepOver(std::shared_ptr<Breakpoint> &);
};
} // namespace mad

#endif /* end of include guard: BREAKPOINTS_HPP_SKXFYM5R */
