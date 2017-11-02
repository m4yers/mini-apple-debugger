#ifndef BREAKPOINTSCONTROL_HPP_DLGATL0P
#define BREAKPOINTSCONTROL_HPP_DLGATL0P

// System
#include <unistd.h>

// Std
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>

// MAD
#include "MAD/MachMemory.hpp"
#include "MAD/MachProcess.hpp"
#include "MAD/Utils.hpp"

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
// Here is an example of this scheme:
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

using AddressType = vm_address_t;

//-----------------------------------------------------------------------------
// Seeds
//-----------------------------------------------------------------------------

// The result of breakpoint callback.
enum class BreakpointCallbackReturn : unsigned {
  // A callback can ask to continue execution of the program.
  CONTINUE = 0x1,

  // Or it can ask for a break and give a user change to do something. If there
  // are several callbacks, it takes only one BREAK return result to stop
  // execution.
  BREAK = 0x2,

  ALL = CONTINUE | BREAK
};

ENUM_BITMASK_DEFINE_ALL(BreakpointCallbackReturn);

using BreakpointByAddressCallback_t =
    std::function<BreakpointCallbackReturn(AddressType)>;
using BreakpointBySymbolNameCallback_t =
    std::function<BreakpointCallbackReturn(std::string)>;

// These are Seed types, and this is what user limited to. Every seed can be
// placed even if the target does not yet exist. Every non-exact(e.g. regex)
// seed is permanently pending. Other will pend until a proper target is
// found.
enum class SeedType {
  // A virtual address to break on. Controller does ASLR correction.
  ADDRESS,

  // A symbol to break on. The much must be exact.
  SYMBOL,

  // Breaks on a line in a file
  LINE,

  // Breaks on symbols that match a regular expression
  REGEX,

  // Breaks on all symbols that belong to a particular class
  CLASS,

  // Breaks on all symbols that belong to a particular file
  FILE,
};

// This policy dictates what to do with a seed in the pending set upon its
// instantiating.
enum class SeedPendingPolicy { REMOVE, KEEP };

class Seed {
public:
  SeedType Type;
  SeedPendingPolicy PendingPolicy;

  // This is a user property. Any seed can be deactivated but not removed.
  // Every its v-point is destroyed as well, and all referenced a-points'
  // counters decremented.
  bool IsActive;

  Seed(SeedType Type, SeedPendingPolicy PendingPolicy)
      : Type(Type), PendingPolicy(PendingPolicy), IsActive(true) {}

  virtual BreakpointCallbackReturn InvokeCallback() = 0;
};
class SeedAddress : public Seed {
public:
  AddressType Address;
  BreakpointByAddressCallback_t Callback;
  SeedAddress(AddressType Address, BreakpointByAddressCallback_t Callback)
      : Seed(SeedType::ADDRESS, SeedPendingPolicy::REMOVE), Address(Address),
        Callback(Callback) {}
  BreakpointCallbackReturn InvokeCallback() { return Callback(Address); }
};
class SeedSymbolName : public Seed {
public:
  std::string SymbolName;
  BreakpointBySymbolNameCallback_t Callback;
  SeedSymbolName(std::string SymbolName,
                 BreakpointBySymbolNameCallback_t Callback)
      : Seed(SeedType::SYMBOL, SeedPendingPolicy::REMOVE),
        SymbolName(SymbolName), Callback(Callback) {}
  BreakpointCallbackReturn InvokeCallback() { return Callback(SymbolName); }
};

using Seed_sp = std::shared_ptr<Seed>;
using SeedAddress_sp = std::shared_ptr<SeedAddress>;
using SeedSymbolName_sp = std::shared_ptr<SeedSymbolName>;

//-----------------------------------------------------------------------------
// Virtual breakpoints
//-----------------------------------------------------------------------------
class VirtualPoint {};
class VirtualPointAddress : public VirtualPoint {
public:
  AddressType Address;
  VirtualPointAddress(AddressType Address) : Address(Address) {}
};
class VirtualPointSymbol : public VirtualPoint {
public:
  using SymbolType_sp = std::shared_ptr<MachOParser64::MachOSymbolTableEntry>;
  SymbolType_sp Symbol;
  VirtualPointSymbol(SymbolType_sp &Symbol) : Symbol(Symbol) {}
};

using VPoint_sp = std::shared_ptr<VirtualPoint>;
using VPointAddress_sp = std::shared_ptr<VirtualPointAddress>;
using VPointSymbol_sp = std::shared_ptr<VirtualPointSymbol>;

//-----------------------------------------------------------------------------
// Actual breakpoints
//-----------------------------------------------------------------------------
class ActualBreakpoint {
public:
  unsigned Count;

  ActualBreakpoint() : Count(0) {}
  ~ActualBreakpoint() {}

  virtual bool Enable() = 0;
  virtual bool Disable() = 0;

  bool IsActive() { return Count > 0; }

  bool Up();
  bool Down();
};
class ActualPointSoftware : public ActualBreakpoint {
public:
  MachMemory &Memory;
  AddressType Address;
  uintptr_t Original;

  bool Enable() override;
  bool Disable() override;

  ActualPointSoftware(AddressType Address, MachMemory &Memory)
      : Memory(Memory), Address(Address) {}
};

using APoint_sp = std::shared_ptr<ActualBreakpoint>;
using APointSoftware_sp = std::shared_ptr<ActualPointSoftware>;

//-----------------------------------------------------------------------------
// Controller
//-----------------------------------------------------------------------------
class BreakpointsControl {
  std::set<Seed_sp> AllSeeds;
  std::set<Seed_sp> PendingSeeds;
  std::map<AddressType, SeedAddress_sp> SeedsByAddress;
  std::map<std::string, SeedSymbolName_sp> SeedsBySymbolName;

  std::set<VPoint_sp> AllVPoints;
  std::map<AddressType, VPointAddress_sp> VPointsByAddress;
  std::map<VirtualPointSymbol::SymbolType_sp, VPointSymbol_sp> VPointsBySymbol;

  // These two MUST stay in sync
  std::map<Seed_sp, std::set<VPoint_sp>> SeedToVPoints;
  std::map<VPoint_sp, std::set<Seed_sp>> VPointToSeeds;

  std::set<APoint_sp> AllAPoints;
  std::map<AddressType, APoint_sp> APointsByAddress;

  // These two MUST stay in sync
  std::map<VPoint_sp, APoint_sp> VPointToAPoint;
  std::map<APoint_sp, std::set<VPoint_sp>> APointToVPoints;

  std::shared_ptr<MachProcess> Process;

private:
  APoint_sp GetOrCreateActualBreakpoint(AddressType Address);
  APoint_sp GetActualBreakpointAtAddress(AddressType Address);
  void TryDestroyActualBreakpoint(APoint_sp &);

  bool TryInstantiateSeedAddress(const SeedAddress_sp &);
  void DestroySeedAddress(const SeedAddress_sp &);

  bool TryInstantiateSeedSymbolName(const SeedSymbolName_sp &);
  void DestroySeedSymbolName(const SeedSymbolName_sp &);

  bool TryToInstantiatePendingSeed(const Seed_sp &);
  void DestroySeed(const Seed_sp &);

  void TryToInstantiateAllPendingSeeds();

public:
  void Attach(std::shared_ptr<MachProcess> Process);
  void Detach();

  bool AddBreakpointByAddress(AddressType Address,
                              BreakpointByAddressCallback_t);
  bool RemoveBreakpointByAddress(AddressType Address);

  bool AddBreakpointBySymbolName(std::string SymbolName,
                                 BreakpointBySymbolNameCallback_t);
  bool RemoveBreakpointBySymbolName(std::string SymbolName);

  // These two methods must be called in sequance. CheckBreakpoints modifies
  // program counter so it points at he breakpoint that stopped program
  // execution. StepOverCurrentBreakpointIfAny steps over it without removing.
  bool CheckBreakpoints();
  bool StepOverCurrentBreakpointIfAny();

  void PrintStats();
};

} // namespace mad

#endif /* end of include guard: BREAKPOINTSCONTROL_HPP_DLGATL0P */
