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
        TryToInstantiateAllPendingSeeds();
        RemoveBreakpointBySymbolName("__dyld_debugger_notification");
        return BreakpointCallbackReturn::CONTINUE;
      });
}

void BreakpointsControl::Detach(bool IsProcessValid) {
  // 1. Disable all active breakpoints
  if (IsProcessValid) {
    for (auto &Point : AllAPoints) {
      if (Point->IsActive()) {
        Point->Disable();
      }
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
APoint_sp
BreakpointsControl::GetActualBreakpointAtAddress(AddressType Address) {
  if (!APointsByAddress.count(Address)) {
    return nullptr;
  }
  return APointsByAddress.at(Address);
}
void BreakpointsControl::TryDestroyActualBreakpoint(APoint_sp &A) {
  if (A->IsActive()) {
    return;
  }

  auto AP = reinterpret_cast<ActualPointSoftware *>(A.get());
  // auto APS = std::static_pointer_cast<ActualPointSoftware>(A);
  APointToVPoints.erase(A);
  APointsByAddress.erase(AP->Address);
  AllAPoints.erase(A);
}

bool BreakpointsControl::TryInstantiateSeedAddress(const SeedAddress_sp &) {
  mad_not_implemented();
  return false;
}
void BreakpointsControl::DestroySeedAddress(const SeedAddress_sp &) {
  mad_not_implemented();
}

bool BreakpointsControl::TryInstantiateSeedSymbolName(
    const SeedSymbolName_sp &S) {
  if (!Process) {
    return false;
  }

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
void BreakpointsControl::DestroySeedSymbolName(const SeedSymbolName_sp &S) {
  for (auto &VPoint : SeedToVPoints[S]) {
    auto V = std::static_pointer_cast<VirtualPointSymbol>(VPoint);
    VPointToSeeds[V].erase(S);
    auto &A = VPointToAPoint[V];

    VPointToAPoint.erase(V);
    APointToVPoints[A].erase(V);

    A->Down();
    TryDestroyActualBreakpoint(A);

    VPointsBySymbol.erase(V->Symbol);
    AllVPoints.erase(V);
  }
  SeedToVPoints.erase(S);

  SeedsBySymbolName.erase(S->SymbolName);
}

bool BreakpointsControl::TryToInstantiatePendingSeed(const Seed_sp &S) {
  assert(PendingSeeds.count(S));

  bool Instantiated = false;

  switch (S->Type) {
  case SeedType::ADDRESS: {
    if (!TryInstantiateSeedAddress(std::static_pointer_cast<SeedAddress>(S))) {
      return false;
    }
    Instantiated = true;
    break;
  }
  case SeedType::SYMBOL: {
    if (!TryInstantiateSeedSymbolName(
            std::static_pointer_cast<SeedSymbolName>(S))) {
      return false;
    }
    Instantiated = true;
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

  if (Instantiated && S->PendingPolicy == SeedPendingPolicy::REMOVE) {
    PendingSeeds.erase(S);
  }

  return true;
}
void BreakpointsControl::DestroySeed(const Seed_sp &S) {
  switch (S->Type) {
  case SeedType::ADDRESS: {
    DestroySeedAddress(std::static_pointer_cast<SeedAddress>(S));
  }
  case SeedType::SYMBOL: {
    DestroySeedSymbolName(std::static_pointer_cast<SeedSymbolName>(S));
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

  PendingSeeds.erase(S);
  AllSeeds.erase(S);
}
void BreakpointsControl::TryToInstantiateAllPendingSeeds() {
  auto ClonePending = PendingSeeds;
  for (auto &S : ClonePending) {
    TryToInstantiatePendingSeed(S);
  }
}

//-----------------------------------------------------------------------------
// Controller API
//-----------------------------------------------------------------------------
bool BreakpointsControl::AddBreakpointByAddress(vm_address_t,
                                                BreakpointByAddressCallback_t) {
  mad_not_implemented();
  return false;
}
bool BreakpointsControl::RemoveBreakpointByAddress(vm_address_t) {
  mad_not_implemented();
  return false;
}

bool BreakpointsControl::AddBreakpointBySymbolName(
    std::string SymbolName, BreakpointBySymbolNameCallback_t Callback) {
  if (SeedsBySymbolName.count(SymbolName)) {
    PRINT_DEBUG("Breakpoint on", SymbolName, "already exists");
    return false;
  }

  auto S = std::make_shared<SeedSymbolName>(SymbolName, Callback);
  SeedsBySymbolName.emplace(SymbolName, S);
  AllSeeds.insert(S);

  PendingSeeds.insert(S);
  TryToInstantiatePendingSeed(S);

  return true;
}
bool BreakpointsControl::RemoveBreakpointBySymbolName(std::string SymbolName) {
  if (!SeedsBySymbolName.count(SymbolName)) {
    PRINT_DEBUG("Breakpoint on", SymbolName, "does not exist");
    return false;
  }

  auto S = SeedsBySymbolName.at(SymbolName);
  DestroySeed(S);

  return true;
}
bool BreakpointsControl::CheckBreakpoints() {
  auto &Thread = Process->GetTask().GetThreads().front();
  Thread.GetStates();
  auto Address = Thread.ThreadState64()->__rip - BREAKPOINT_SIZE;
  auto A = GetActualBreakpointAtAddress(Address);
  if (!A) {
    PRINT_DEBUG("Weird, no active breakpoints here at", HEX(Address));
    return true;
  }

  // Sanity check
  assert(A->IsActive());

  // Move program counter to breakpoint start address
  Thread.ThreadState64()->__rip = Address;
  Thread.SetStates();

  auto NextBits = BreakpointCallbackReturn::CONTINUE;
  auto APointToVPointsAClone = APointToVPoints.at(A);
  for (auto &V : APointToVPointsAClone) {
    auto VPointToSeedsVClone = VPointToSeeds.at(V);
    for (auto &S : VPointToSeedsVClone) {
      NextBits |= S->InvokeCallback();
    }
  }

  // The program execution will continue if there are no BREAK callback results
  bool Continue = (NextBits & BreakpointCallbackReturn::BREAK) !=
                  BreakpointCallbackReturn::BREAK;

  return Continue;
}
bool BreakpointsControl::StepOverCurrentBreakpointIfAny() {
  auto &Thread = Process->GetTask().GetThreads().front();
  Thread.GetStates();
  auto Address = Thread.ThreadState64()->__rip;
  auto A = GetActualBreakpointAtAddress(Address);
  bool Active = A && A->IsActive();
  if (Active) {
    A->Disable();
  }

  Process->Step();

  if (Active) {
    A->Enable();
  }

  return true;
}

void BreakpointsControl::PrintStats() {}
