#ifndef BREAKPOINTS_HPP_SKXFYM5R
#define BREAKPOINTS_HPP_SKXFYM5R

// System
#include <unistd.h>

// Std
#include <experimental/any> // Rewrite using variant later...
#include <functional>
#include <map>
#include <memory>
#include <set>

// MAD
#include "MAD/MachMemory.hpp"
#include "MAD/MachProcess.hpp"

// This class-set describes breakpoints you can set during mad-debugging. There
// is three levels to implement breakpoints:
//
//   - Input/Pending breakpoints, here they are called Seeds. Their role is to
//     provide high level breakpoint support, e.g. break-on-symbol,
//     break-on-address, break-on-regex etc.
//   - Virtual breakpoints, this is what user-layer breakpoints resolve into.
//     Because there is a possibility of user breakpoints overlap there is
//     many-to-many relationship between Seeds and v-points.
//   - Actual breakpoints, these are actual software breakpoints mad is using
//     to stop execution. There can be only one per address, but multiple
//     v-points can reference a single a-point; if the count value is > 0
//     the breakpoint is set and active, otherwise it is not.
//
// Here is aa example of this scheme:
//
//    Input                       Virtual               Actual
//
//  [1] break-on-address ------ [1] An Address ------ [1] Count: 2 -> Enabled
//                                              ____/
//                                             /
//  [2] break-on-symbol  ------ [2] A Symbol        - [2] Count: 1 -> Enabled
//                        ____/                ____/
//                       /                    /
//  [3] break-on-regex   ------ [3] A Symbol -        [3] Count: 0 -> Disabled
namespace mad {

// FIXME Need a better system here. How to handle user/system breakpoints?
enum class BreakpointCallbackReturn {
  CONTINUE,
  MOVE_TO_BREAKPOINT,
  REMOVE,
  REMOVE_CONTINUE,
  STEP_OVER,
  STEP_OVER_CONTINUE
};

using BreakpointByAddressCallback_t =
    std::function<BreakpointCallbackReturn(vm_address_t)>;
using BreakpointBySymbolNameCallback_t =
    std::function<BreakpointCallbackReturn(std::string)>;

enum class SeedType { ADDRESS, SYMBOL };
class Seed {
public:
  SeedType Type;
  bool IsActive;
  Seed(SeedType Type) : Type(Type), IsActive(true) {}
};
class SeedAddress : public Seed {
public:
  BreakpointByAddressCallback_t Callback;
  vm_address_t Address;
  SeedAddress() : Seed(SeedType::ADDRESS) {}
};
class SeedSymbolName : public Seed {
public:
  BreakpointBySymbolNameCallback_t Callback;
  std::string SymbolName;
  SeedSymbolName() : Seed(SeedType::SYMBOL) {}
};

using Seed_sp = std::shared_ptr<Seed>;
using SeedAddress_sp = std::shared_ptr<SeedAddress>;
using SeedSymbolName_sp = std::shared_ptr<SeedSymbolName>;

enum class BreakpointStatus { UNKNOWN, PENDING, SET, DISABLED };

// TODO: Add Virtual breakpoints overlay to handle many-to-many seed/breakpoints rel.
class Breakpoint {
public:
  Seed_sp Seed;
  MachMemory &Memory;
  uint64_t Address;
  uintptr_t Original;
  bool IsActive;

public:
  Breakpoint(Seed_sp Seed, MachMemory &Memory, vm_address_t Address)
      : Seed(Seed), Memory(Memory), Address(Address), IsActive(false) {}
  // Normally destruction must lead to Breakpoint removal, but during detach
  // process no longer exists and mach port is no longer valid. At this point
  // we just do nothing.  Other Breakpoint removal cases are handled explicitly
  // throughout code
  // ~Breakpoint() {
  //   if (IsActive) {
  //     Disable();
  //   }
  // }

  auto GetAddress() { return Address; }

  bool Enable();
  bool Disable();
};

using Breakpoint_sp = std::shared_ptr<Breakpoint>;

class Breakpoints {
  std::shared_ptr<MachProcess> Process;

  // All available seeds
  std::vector<Seed_sp> Seeds;

  // These are type seed type specific maps, each seed is access via its
  // distinc characteristic, e.g. address or symbol name.
  std::map<uint64_t, SeedAddress_sp> SeedsByAddress;
  std::map<std::string, SeedSymbolName_sp> SeedsBySymbolName;

  // Maps active/idle seeds to existing breakpoints. There can be multiple
  // breakpoints belonging to a single seed, e.g. regex matching.
  std::map<Seed_sp, std::vector<Breakpoint_sp>> SeedToBreapoints;

  // These are seeds that yet to become breakpoints, given they are still
  // active.
  std::set<Seed_sp> PendingSeeds;

  std::map<uint64_t, Breakpoint_sp> BreakpointsByAddress;
  std::map<uint64_t, std::vector<Breakpoint_sp>> BreakpointsByPage;

private:
  Seed_sp GetSeedByAddress(uint64_t, BreakpointByAddressCallback_t);
  Seed_sp GetSeedBySymbolName(std::string, BreakpointBySymbolNameCallback_t);

  bool ApplyBreakpoint(Breakpoint_sp &);
  bool ApplyPendingSeed(Seed_sp &);

  void RemoveBreakpoint(Breakpoint_sp &);

  void HandleDebuggerNotification();
  void HandleMoveToBreakPoint(Breakpoint_sp &);
  void HandleRemove(Breakpoint_sp &);
  void HandleStepOver(Breakpoint_sp &);

  Breakpoint_sp GetCurrentBreakpoint();

public:
  bool AddBreakpointByAddress(vm_address_t Address,
                              BreakpointByAddressCallback_t);
  BreakpointStatus AddBreakpointBySymbolName(std::string SymbolName,
                                             BreakpointBySymbolNameCallback_t);
  void RemoveBreakpointBySymbolName(std::string SymbolName);

  void Attach(std::shared_ptr<MachProcess> Process);
  void Detach();
  bool CheckBreakpoints();
  bool StandingOnBreakpoint();
  bool StepOverCurrentBreakpoint();
};
} // namespace mad

#endif /* end of include guard: BREAKPOINTS_HPP_SKXFYM5R */
