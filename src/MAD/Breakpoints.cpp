#include "MAD/Breakpoints.hpp"

using namespace mad;

#define BREAKPOINT_INST 0xCC
#define BREAKPOINT_MASK ~0xFF
#define BREAKPOINT_SIZE 1ul

bool Breakpoint::Enable() {
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

bool Breakpoint::Disable() {
  if (Memory.Write(Address, (vm_offset_t)&Original, sizeof(Original)) !=
      sizeof(Original)) {
    Error Err(MAD_ERROR_BREAKPOINT);
    Err.Log("Could not remove breakpoint at", HEX(Address));
    return false;
  }

  return true;
}

void Breakpoints::AddBreakpoint(std::shared_ptr<Breakpoint> BP) {
  auto &Memory = Process.GetTask().GetMemory();
  auto PageSize = Memory.GetPageSize();
  auto PageMask = ~(PageSize - 1);
  auto Address = BP->GetAddress();
  BreakpointsByPage[Address & PageMask].push_back(BP);
  BreakpointsByAddress.insert({Address, BP});
  // FIXME: Add proper handling of breakpoints on code that is not loaded yet
  BP->Enable();
}

void Breakpoints::AddBreakpointByAddress(vm_address_t Address,
                                         BreakpointByAddress_t Callback) {}

void Breakpoints::AddBreakpointBySymbolName(std::string SymbolName,
                                            BreakpointBySymbolName_t Callback) {
  for (auto &Image : Process.GetImagess()) {
    auto &SymbolTable = Image->GetSymbolTable();
    auto Symbol = SymbolTable.GetSymbolByName(SymbolName);
    if (!Symbol) {
      continue;
    }

    auto &Memory = Process.GetTask().GetMemory();
    BreakpointSeedSymbol Seed;
    Seed.Callback = Callback;
    Seed.Symbol = Symbol;
    auto NewBreakpoint = std::make_shared<Breakpoint>(
        BreakpointType::Symbol, Seed, Memory, Symbol->Value);
    AddBreakpoint(NewBreakpoint);
    return;
  }
}

void AddBreakpointBySymbol(std::string SymbolName, BreakpointBySymbolName_t) {}

void Breakpoints::PreRun() {
  // We search Dynamic Linker in-memory image for the specific function symbol.
  // It is used to sync via breakpoint with a debugger. Once debugger attaches
  // to the process it will stop at _start symbol of the Dynamic Linker, in
  // order to skip the DyLD code we need to setup a breakpoint at this
  // function. This stub function is run just before executing any user code
  // including shared library's init code and C++ static constructors.
  AddBreakpointBySymbolName("__dyld_debugger_notification",
                            [this](std::string) {
                              HandleDebuggerNotification();
                              return BreakpointCallbackReturn::REMOVE_CONTINUE;
                            });
}

void Breakpoints::PostRun() {}

void Breakpoints::HandleDebuggerNotification() {
  PRINT_DEBUG("HandleDebuggerNotification");
  // FIXME: Try to assing all available breakpoints to user code
}

void Breakpoints::HandleRemove(std::shared_ptr<Breakpoint> &BP) {
  BP->Disable();
  auto &Thread = Process.GetTask().GetThreads().front();
  Thread.GetStates();
  Thread.ThreadState64()->__rip -= BREAKPOINT_SIZE;
  Thread.SetStates();
}

void Breakpoints::HandleStepOver(std::shared_ptr<Breakpoint> &) {}

bool Breakpoints::CheckBreakpoints() {
  auto &Thread = Process.GetTask().GetThreads().front();
  Thread.GetStates();
  auto Address = Thread.ThreadState64()->__rip - BREAKPOINT_SIZE;
  if (!BreakpointsByAddress.count(Address)) {
    PRINT_DEBUG("WEIRD, no breakpoints at", HEX(Address));
    auto Start = Thread.ThreadState64()->__rip - 10;
    char mem[50];
    Process.GetTask().GetMemory().Read(Start, 100, mem);
    for (int i = 0; i < 50; ++i) {
      PRINT_DEBUG("MEM:", HEX((Start + i)), HEX(mem[i]));
    }
    return true;
  }

  BreakpointCallbackReturn WhatNext;
  auto BP = BreakpointsByAddress.at(Address);
  auto Seed = BP->Seed;
  switch (BP->Type) {
  case BreakpointType::Address:
    mad_unreachable("Not implemented");
    break;
  case BreakpointType::Symbol:
    auto SymbolSeed = std::experimental::any_cast<BreakpointSeedSymbol>(&Seed);
    WhatNext = SymbolSeed->Callback(SymbolSeed->Symbol->Name);
    break;
  }

  switch (WhatNext) {
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
