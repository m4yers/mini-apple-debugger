#include "MAD/Breakpoints.hpp"

using namespace mad;

#define BREAKPOINT_INST 0xCC
#define BREAKPOINT_MASK ~0xFF
#define BREAKPOINT_SIZE 1ul

bool Breakpoint::Enable() {
  assert(!IsActive);

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

  IsActive = true;
  return true;
}

bool Breakpoint::Disable() {
  assert(IsActive);

  // TODO before writing, read current value so it won't overwrite other breakpoint
  if (Memory.Write(Address, (vm_offset_t)&Original, sizeof(Original)) !=
      sizeof(Original)) {
    Error Err(MAD_ERROR_BREAKPOINT);
    Err.Log("Could not remove breakpoint at", HEX(Address));
    return false;
  }

  IsActive = false;
  return true;
}

Seed_sp Breakpoints::GetSeedByAddress(uint64_t, BreakpointByAddressCallback_t) {
  mad_not_implemented();
}

Seed_sp
Breakpoints::GetSeedBySymbolName(std::string Name,
                                 BreakpointBySymbolNameCallback_t Callback) {
  if (SeedsBySymbolName.count(Name)) {
    return std::static_pointer_cast<Seed>(SeedsBySymbolName.at(Name));
  }

  SeedSymbolName_sp SymbolSeed = std::make_shared<SeedSymbolName>();
  SymbolSeed->Callback = Callback;
  SymbolSeed->SymbolName = Name;
  SeedsBySymbolName.emplace(Name, SymbolSeed);
  Seeds.push_back(SymbolSeed);
  return std::static_pointer_cast<Seed>(SymbolSeed);
}

bool Breakpoints::ApplyBreakpoint(Breakpoint_sp &BP) {
  if (BreakpointsByAddress.count(BP->Address)) {
    return false;
  }

  PRINT_DEBUG("ADD BP AT", HEX(BP->GetAddress()));
  auto &Memory = Process->GetTask().GetMemory();
  auto PageSize = Memory.GetPageSize();
  auto PageMask = ~(PageSize - 1);
  auto Address = BP->GetAddress();
  BreakpointsByPage[Address & PageMask].push_back(BP);
  BreakpointsByAddress.insert({Address, BP});
  BP->Enable();
  return true;
}

void Breakpoints::RemoveBreakpoint(Breakpoint_sp &BP) {
  if (BP->IsActive) {
    BP->Disable();
  }
  auto &Memory = Process->GetTask().GetMemory();
  auto PageSize = Memory.GetPageSize();
  auto PageMask = ~(PageSize - 1);
  auto Address = BP->GetAddress();
  BreakpointsByPage.erase(Address & PageMask);
  BreakpointsByAddress.erase(Address);
}

bool Breakpoints::ApplyPendingSeed(Seed_sp &Seed) {
  if (!Process) {
    return false;
  }

  switch (Seed->Type) {
  case SeedType::ADDRESS: {
    PRINT_DEBUG("ADDRESS");
    mad_not_implemented();
    break;
  }
  case SeedType::SYMBOL: {
    auto SymbolSeed = std::static_pointer_cast<SeedSymbolName>(Seed);
    for (auto &Image : Process->GetImagess()) {
      auto &SymbolTable = Image->GetSymbolTable();
      auto Symbol = SymbolTable.GetSymbolByName(SymbolSeed->SymbolName);
      if (!Symbol) {
        continue;
      }

      auto &Memory = Process->GetTask().GetMemory();
      auto NewBreakpoint =
          std::make_shared<Breakpoint>(Seed, Memory, Symbol->Value);
      if (ApplyBreakpoint(NewBreakpoint)) {
        PendingSeeds.erase(Seed);
        SeedToBreapoints[Seed].push_back(NewBreakpoint);
        return true;
      }
    }
    break;
  }
  }

  return false;
}

bool Breakpoints::AddBreakpointByAddress(
    vm_address_t Address, BreakpointByAddressCallback_t Callback) {
  mad_not_implemented();
}

BreakpointStatus Breakpoints::AddBreakpointBySymbolName(
    std::string SymbolName, BreakpointBySymbolNameCallback_t Callback) {
  if (SeedsBySymbolName.count(SymbolName)) {
    PRINT_DEBUG("This breakpoint already exists");
    // TODO: return proper status: set/disabled/pending
    return BreakpointStatus::SET;
  }
  auto Seed = GetSeedBySymbolName(SymbolName, Callback);
  assert(!SeedToBreapoints[Seed].size() &&
         "There must not be any breakpoints for this seed yet.");

  PendingSeeds.insert(Seed);

  if (ApplyPendingSeed(Seed)) {
    return BreakpointStatus::SET;
  } else {
    return BreakpointStatus::PENDING;
  }
}

void Breakpoints::RemoveBreakpointBySymbolName(std::string SymbolName) {
  if (!SeedsBySymbolName.count(SymbolName)) {
    PRINT_DEBUG("There is no such breakpoint");
  }
  auto S = SeedsBySymbolName[SymbolName];
  if (SeedToBreapoints.count(S)) {
    for (auto &BP : SeedToBreapoints[S]) {
      RemoveBreakpoint(BP);
    }
    SeedToBreapoints.erase(S);
  }
  PendingSeeds.erase(S);
  SeedsBySymbolName.erase(SymbolName);
}

void Breakpoints::HandleMoveToBreakPoint(Breakpoint_sp &BP) {
  auto &Thread = Process->GetTask().GetThreads().front();
  Thread.GetStates();
  Thread.ThreadState64()->__rip = BP->GetAddress();
  Thread.SetStates();
}

void Breakpoints::HandleRemove(Breakpoint_sp &BP) {
  HandleMoveToBreakPoint(BP);
  // TODO proper removal, if its seed does not have other breaks -> remove it
  // HMM Do i really need these handlers?
  BP->Disable();
}

void Breakpoints::HandleStepOver(Breakpoint_sp &BP) {
  HandleMoveToBreakPoint(BP);
  StepOverCurrentBreakpoint();
}

void Breakpoints::Attach(std::shared_ptr<MachProcess> Proc) {
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

void Breakpoints::Detach() {
  SeedToBreapoints.clear();
  BreakpointsByAddress.clear();
  BreakpointsByPage.clear();

  // Any active seed during detaching must be placed into the pending set so
  // the next process run could use these active breaks.
  for (auto &Seed : Seeds) {
    if (Seed->IsActive) {
      PendingSeeds.insert(Seed);
    }
  }

  Process = nullptr;
}

void Breakpoints::HandleDebuggerNotification() {
  auto ClonePendingSeeds = PendingSeeds;
  for (auto Seed : ClonePendingSeeds) {
    if (ApplyPendingSeed(Seed)) {
    }
  }
}

bool Breakpoints::CheckBreakpoints() {
  auto &Thread = Process->GetTask().GetThreads().front();
  Thread.GetStates();
  auto Address = Thread.ThreadState64()->__rip - BREAKPOINT_SIZE;
  if (!BreakpointsByAddress.count(Address)) {
    PRINT_DEBUG("WEIRD, no breakpoints at", HEX(Address));
    auto Start = Thread.ThreadState64()->__rip - 10;
    char mem[50];
    Process->GetTask().GetMemory().Read(Start, 100, mem);
    for (int i = 0; i < 50; ++i) {
      PRINT_DEBUG("MEM:", HEX((Start + i)), HEX(mem[i]));
    }
    return true;
  }

  auto WhatNext = BreakpointCallbackReturn::MOVE_TO_BREAKPOINT;
  auto BP = BreakpointsByAddress.at(Address);
  auto Seed = BP->Seed;
  switch (Seed->Type) {
  case SeedType::ADDRESS:
    mad_not_implemented();
    break;
  case SeedType::SYMBOL:
    auto SymbolSeed = std::static_pointer_cast<SeedSymbolName>(Seed);
    WhatNext = SymbolSeed->Callback(SymbolSeed->SymbolName);
    break;
  }

  switch (WhatNext) {
  case BreakpointCallbackReturn::CONTINUE:
    return true;
  case BreakpointCallbackReturn::MOVE_TO_BREAKPOINT:
    HandleMoveToBreakPoint(BP);
    return false;
  case BreakpointCallbackReturn::REMOVE:
    HandleRemove(BP);
    return false;
  case BreakpointCallbackReturn::REMOVE_CONTINUE:
    HandleRemove(BP);
    return true;
  case BreakpointCallbackReturn::STEP_OVER:
    HandleStepOver(BP);
    return false;
  case BreakpointCallbackReturn::STEP_OVER_CONTINUE:
    HandleStepOver(BP);
    return true;
  }
}

Breakpoint_sp Breakpoints::GetCurrentBreakpoint() {
  auto &Thread = Process->GetTask().GetThreads().front();
  Thread.GetStates();
  auto Address = Thread.ThreadState64()->__rip;
  if (BreakpointsByAddress.count(Address) &&
      BreakpointsByAddress[Address]->IsActive) {
    return BreakpointsByAddress[Address];
  }
  return nullptr;
}

bool Breakpoints::StandingOnBreakpoint() {
  return GetCurrentBreakpoint() != nullptr;
}

bool Breakpoints::StepOverCurrentBreakpoint() {
  auto BP = GetCurrentBreakpoint();
  if (BP) {
    BP->Disable();
  }

  // FIXME: Handle status
  Process->Step();

  if (BP) {
    BP->Enable();
  }
  return true;
}
