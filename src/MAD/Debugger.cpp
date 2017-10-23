#include <mach/mach.h>
#include <sys/types.h>

#include <sys/ptrace.h>

#include <iostream>

#include <dlfcn.h>
#include <mach-o/dyld.h>

#include "MAD/Debugger.hpp"
#include "MAD/MachTask.hpp"
#include "MAD/MachThread.hpp"

#include "MAD/Debug.hpp"

// NOTE: Keep this in sync with struct __darwin_x86_thread_state64
char regs_x86_64[][7] = {"rax", "rbx", "rcx", "rdx",    "rdi", "rsi", "rbp",
                         "rsp", "r8",  "r9",  "r10",    "r11", "r12", "r13",
                         "r14", "r15", "rip", "rflags", "cs",  "fs",  "gs"};

using namespace mad;

void Debugger::StartDebugging() {
  PRINT_DEBUG("Debugger started");

  int wait_status;
  wait(&wait_status);

  if (!Process.Attach()) {
    PRINT_ERROR("Failed to attach");
    exit(1);
  }
  PRINT_DEBUG("Attached...");

  // We search Dynamic Linker in-memory image for the specific function symbol.
  // It is used to sync via breakpoint with a debugger. Once debugger attaches
  // to the process it will stop at _start symbol of the Dynamic Linker, in
  // order to skip the DyLD code we need to setup a breakpoint at this
  // function. This stub function is run just before executing any user code
  // including shared library's init code and C++ static constructors.
  auto DyLinkerImage = Process.GetDynamicLinkerImage();
  auto DyLinkerNotify = DyLinkerImage->GetSymbolTable().GetSymbolByName(
      "__dyld_debugger_notification");

  auto DyLinkerNotifyBreakpoint = CreateBreakpoint(DyLinkerNotify->Value);
  if (!DyLinkerNotifyBreakpoint->Enable()) {
    Error Err(MAD_ERROR_BREAKPOINT);
    Err.Log("Could not set break at __dyld_debugger_notification");
    return;
  }

  // Wait till we hit that breakpoint
  ptrace(PT_CONTINUE, Process.GetPID(), (caddr_t)1, 0);
  wait(&wait_status);

  // Remove breakpoint
  if (!DyLinkerNotifyBreakpoint->Disable()) {
    Error Err(MAD_ERROR_BREAKPOINT);
    Err.Log("Could not disable break at __dyld_debugger_notification");
    return;
  }

  // Move to the start of the symbol
  auto &Thread = Task.GetThreads().front();
  Thread.GetStates();
  Thread.ThreadState64()->__rip = DyLinkerNotifyBreakpoint->GetAddress();
  Thread.SetStates();

  PRINT_DEBUG("SYM: ", HEX(DyLinkerNotifyBreakpoint->GetAddress()));
  PRINT_DEBUG("RIP: ", HEX(Thread.ThreadState64()->__rip));

  auto HelloImage = Process.GetImagesByName(
      "/Users/m4yers/Development/Projects/mini-apple-debugger/hello");
  auto Main = HelloImage->GetSymbolTable().GetSymbolByName("_main");
  auto MainBreakpoint = CreateBreakpoint(Main->Value);
  if (!MainBreakpoint->Enable()) {
    PRINT_ERROR("Oh SNAP!");
    exit(2);
  }

  ptrace(PT_CONTINUE, Process.GetPID(), (caddr_t)1, 0);
  wait(&wait_status);

  if (!MainBreakpoint->Disable()) {
    PRINT_ERROR("Oh SNAP! Disable");
    exit(2);
  }

  auto &Thread2 = Task.GetThreads().front();
  Thread2.GetStates();
  Thread2.ThreadState64()->__rip = MainBreakpoint->GetAddress();
  Thread2.SetStates();

  int instructions = 0;
  while (WIFSTOPPED(wait_status)) {
    instructions++;

    auto &Thread3 = Task.GetThreads().front();
    Thread3.GetStates();

    uint64_t Data;
    Memory.Read(Thread3.ThreadState64()->__rip, sizeof(Data), &Data);
    PRINT_DEBUG("RIP: ", HEX(Thread3.ThreadState64()->__rip), " MEM: ", HEX(Data));

    // if (auto threads = Task.GetThreads()) {
    //   for (auto thread : *threads) {
    //     // thread.GetStates();
    //     // std::cout << "Thread " << thread.GetId() << ":" << std::endl;
    //     // for (size_t i = 0; i < sizeof(regs_x86_64) / 7; ++i) {
    //     //   std::cout << regs_x86_64[i] << ": "
    //     //             << (&thread.ThreadState64()->__rax)[i] << std::endl;
    //     // }
    //     thread.GetStates();
    //     // address = thread.ThreadState64()->__rip;
    //     PRINT_DEBUG("T: ", thread.GetId(), ", I: ", icounter,
    //         ", RIP: ", (void *)thread.ThreadState64()->__rip);
    //   }
    // }

    if (ptrace(PT_STEP, Process.GetPID(), (caddr_t)1, 0) < 0) {
      PRINT_ERROR(std::strerror(errno));
      return;
    }

    break;
    wait(&wait_status);
  }

  PRINT_DEBUG("INSTRUCTIONS: ", instructions);
}

std::shared_ptr<Breakpoint> Debugger::CreateBreakpoint(vm_address_t Address) {
  assert(!BreakpointsByAddress.count(Address) && "Breakpoint already exists");

  auto NewBreakpoint = std::make_shared<Breakpoint>(Memory, Address);

  auto PageSize = Memory.GetPageSize();
  auto PageMask = ~(PageSize - 1);
  BreakpointsByPage[Address & PageMask].push_back(NewBreakpoint);

  BreakpointsByAddress.insert({Address, NewBreakpoint});

  return NewBreakpoint;
}

int Debugger::Run() {
  if (auto code = Process.Execute()) {
    return code;
  }

  if (!Process.IsParent()) {
    return 2;
  }

  StartDebugging();

  return 0;
}
