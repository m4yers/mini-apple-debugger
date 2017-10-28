#include "MAD/BreakpointsControl.hpp"

using namespace mad;

//-----------------------------------------------------------------------------
// Actual breakpoints
//-----------------------------------------------------------------------------
#define BREAKPOINT_INST 0xCC
#define BREAKPOINT_MASK ~0xFF
#define BREAKPOINT_SIZE 1ul

bool ActualBreakpoint::Up() {
  Count++;
  if (Count == 1) {
    return Enable();
  }
  return true;
}

bool ActualBreakpoint::Down() {
  assert(Count);
  Count--;
  if (!Count) {
    return Disable();
  }
  return true;
}

bool ActualPointSoftware::Enable() {
  assert(Count);

  if (Memory.Read(Address, sizeof(Original), &Original) != sizeof(Original)) {
    Error Err(MAD_ERROR_BREAKPOINT);
    Err.Log("Could not set breakpoint at", HEX(Address));
    return false;
  }

  decltype(Original) Modified = (Original & BREAKPOINT_MASK) | BREAKPOINT_INST;

  if (Memory.Write(Address, (vm_offset_t)&Modified, sizeof(Modified)) !=
      sizeof(Original)) {
    Error Err(MAD_ERROR_BREAKPOINT);
    Err.Log("Could not set breakpoint at", HEX(Address));
    return false;
  }

  return true;
}

bool ActualPointSoftware::Disable() {
  assert(!Count);

  // TODO before writing, read current value so it won't overwrite other
  // breakpoint
  if (Memory.Write(Address, (vm_offset_t)&Original, sizeof(Original)) !=
      sizeof(Original)) {
    Error Err(MAD_ERROR_BREAKPOINT);
    Err.Log("Could not remove breakpoint at", HEX(Address));
    return false;
  }

  return true;
}

//-----------------------------------------------------------------------------
// Controller
//-----------------------------------------------------------------------------
void BreakpointsControl::Attach(std::shared_ptr<MachProcess> Proc) {
  Process = Proc;
  // We search Dynamic Linker in-memory image for the specific function symbol.
  // It is used to sync via breakpoint with a debugger. Once debugger attaches
  // to the process it will stop at _start symbol of the Dynamic Linker, in
  // order to skip the DyLD code we need to setup a breakpoint at this
  // function. This stub function is run just before executing any user code
  // including shared library's init code and C++ static constructors.
  AddBreakpointBySymbolName(
      "__dyld_debugger_notification", [this](std::string) {
        HandleDebuggerNotification();
        RemoveBreakpointBySymbolName("__dyld_debugger_notification");
        return BreakpointCallbackReturn::CONTINUE;
      });
}

void BreakpointsControl::Detach() {
  // 1. Disable all active breakpoints
  for (auto &Point : AllAPoints) {
    if (Point->IsActive()) {
      Point->Disable();
    }
  }

  // 2. Clear all a-points
  APointToVPoints.clear();
  VPointToAPoint.clear();
  APointsByAddress.clear();
  AllAPoints.clear();

  // 3. Clear all v-points
  VPointToSeeds.clear();
  SeedToVPoints.clear();
  VPointsBySymbol.clear();
  VPointsByAddress.clear();
  AllVPoints.clear();

  // 4. Make all available seeds pending, so that next run of a program can use
  // them
  PendingSeeds = AllSeeds;

  Process = nullptr;
}

APoint_sp BreakpointsControl::GetOrCreateActualBreakpoint(AddressType Address) {
  if (APointsByAddress.count(Address)) {
    return APointsByAddress[Address];
  }

  auto APS = std::make_shared<ActualPointSoftware>(
      Address, Process->GetTask().GetMemory());
  APointsByAddress.emplace(Address, APS);
  AllAPoints.insert(APS);
  return APS;
}

bool BreakpointsControl::TryInstantiateSeedAddress(const SeedAddress_sp &) {
  mad_not_implemented();
  return false;
}
bool BreakpointsControl::TryInstantiateSeedSymbolName(
    const SeedSymbolName_sp &S) {
  VirtualPointSymbol::SymbolType_sp Symbol;
  for (auto &Image : Process->GetImagess()) {
    auto &SymbolTable = Image->GetSymbolTable();
    // TODO There are no HW breakpoints now, so filter out non-code
    // symbols somehow
    if (SymbolTable.HasSymbol(S->SymbolName)) {
      Symbol = SymbolTable.GetSymbolByName(S->SymbolName);
      break;
    }
  }

  if (!Symbol) {
    return false;
  }

  auto A = GetOrCreateActualBreakpoint(Symbol->Value);
  // If we fail at this moment we do not create any v/a points
  if (!A->Up()) {
    return false;
  }

  auto V = std::make_shared<VirtualPointSymbol>(Symbol);
  VPointsBySymbol.emplace(Symbol, V);
  AllVPoints.insert(V);

  // Keep in sync
  SeedToVPoints[S].insert(V);
  VPointToSeeds[V].insert(S);

  // Keep in sync
  VPointToAPoint.emplace(V, A);
  APointToVPoints[A].insert(V);

  return true;
}

bool BreakpointsControl::TryToInstantiatePendingSeed(const Seed_sp &S) {
  assert(PendingSeeds.count(S));

  bool Intantiated = false;

  switch (S->Type) {
  case SeedType::ADDRESS: {
    if (!TryInstantiateSeedAddress(std::const_pointer_cast<SeedAddress>(S))) {
      return false;
    }
    break;
  }
  case SeedType::SYMBOL: {
    if (!TryInstantiateSeedSymbolName(
            std::const_pointer_cast<SeedSymbolName>(S))) {
      return false;
    }
    break;
  }
  case SeedType::LINE: {
    mad_not_implemented();
    break;
  }
  case SeedType::REGEX: {
    mad_not_implemented();
    break;
  }
  case SeedType::CLASS: {
    mad_not_implemented();
    break;
  }
  case SeedType::FILE: {
    mad_not_implemented();
    break;
  }
  }

  if (Intantiated && S->PendingPolicy == SeedPendingPolicy::REMOVE) {
    PendingSeeds.erase(S);
  }

  return true;
}
void BreakpointsControl::TryToInstantiateAllPendingSeeds() {
  auto ClonePending = PendingSeeds;
  for (auto &S : ClonePending) {
    TryToInstantiatePendingSeed(S);
  }
}

void BreakpointsControl::HandleDebuggerNotification() { mad_not_implemented(); }

//-----------------------------------------------------------------------------
// Controller API
//-----------------------------------------------------------------------------
bool BreakpointsControl::AddBreakpointByAddress(vm_address_t Address,
                                                BreakpointByAddressCallback_t) {
  mad_not_implemented();
  return false;
}
void BreakpointsControl::RemoveBreakpointByAddress(vm_address_t Address) {
  mad_not_implemented()
}

bool BreakpointsControl::AddBreakpointBySymbolName(
    std::string SymbolName, BreakpointBySymbolNameCallback_t) {
  if (SeedsBySymbolName.count(SymbolName)) {
    PRINT_DEBUG("Breakpoint on", SymbolName, "already exists");
    return false;
  }

  auto S = std::make_shared<SeedAddress>(SymbolName);
  SeedsBySymbolName.emplace(SymbolName, S);
  AllSeeds.insert(S);

  TryToInstantiatePendingSeed(std::const_pointer_cast<Seed>(S));

  return true;
}
void BreakpointsControl::RemoveBreakpointBySymbolName(std::string SymbolName) {
  mad_not_implemented();
}
