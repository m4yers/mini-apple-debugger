// System
#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <mach/mach.h>
#include <sys/types.h>

#include <sys/ptrace.h>

// Std
#include <iostream>

// MAD
#include "MAD/Debug.hpp"
#include "MAD/Debugger.hpp"
#include "MAD/MachTask.hpp"
#include "MAD/MachThread.hpp"

// NOTE: Keep this in sync with struct __darwin_x86_thread_state64
char regs_x86_64[][7] = {"rax", "rbx", "rcx", "rdx",    "rdi", "rsi", "rbp",
                         "rsp", "r8",  "r9",  "r10",    "r11", "r12", "r13",
                         "r14", "r15", "rip", "rflags", "cs",  "fs",  "gs"};

using namespace mad;

void Debugger::HandleProcessContinue() {
  if (!Process) {
    Prompt.Say("You must run the program first");
    return;
  }

  bool Continue = true;
  while (Continue) {
    BreakpointsCtrl.StepOverCurrentBreakpointIfAny();

    Continue = false;
    auto Status = Process->Continue();
    switch (Status.Type) {
    case MachProcessStatusType::ERROR:
      // To make life under lldb easier...
      if (Status.Error.Flavour == ErrorFlavour::POSIX &&
          Status.Error.Value == EINTR) {
        continue;
      }
      Status.Error.Log("Counld continue process", Process->GetPID());
      HandleProcessStop();
      break;
    case MachProcessStatusType::CONTINUED:
      break;
    case MachProcessStatusType::EXITED:
      Prompt.Say("Program", Exe, "finished with status", Status.ExitStatus);
      HandleProcessStop();
      break;
    case MachProcessStatusType::SIGNALED:
      Prompt.Say("Program", Exe, "has been signaled", Status.TermSignal);
      HandleProcessStop();
      break;
    case MachProcessStatusType::STOPPED:
      // FIXME handle signals
      switch (Status.StopSignal) {
      case SIGTRAP:
        Continue = BreakpointsCtrl.CheckBreakpoints();
        break;
      default:
        PRINT_DEBUG("Unhandled signal", Status.StopSignal);
        mad_unreachable("Other signals not implemented");
      }
      break;
    }
  }
}

void Debugger::HandleProcessRun() {
  if (Process) {
    Prompt.Say("Program", Exe, "is already running...");
    return;
  }

  Process = std::make_unique<MachProcess>(Exe);

  PRINT_DEBUG("Spawning", Exe, "...");
  if (auto code = Process->Execute()) {
    Process = nullptr;
    Error Err(MAD_ERROR_PROCESS, ErrorFlavour::MAD);
    Err.Log("Could not spaw a child process");
    return;
  }
  PRINT_DEBUG("Done");

  if (!Process->IsParent()) {
    Process = nullptr;
    Error Err(MAD_ERROR_PROCESS, ErrorFlavour::MAD);
    Err.Log("Could not execute child program");
    exit(2);
  }

  // sleep(1);
  MachProcessStatus Status;
  Process->Wait(Status);

  if (Status.Error) {
    PRINT_ERROR("Failed to wait");
    exit(1);
  }

  PRINT_DEBUG("Attaching to", Exe, "at", Process->GetPID(), "...");
  if (!Process->Attach()) {
    PRINT_ERROR("Failed to attach");
    exit(1);
  }
  PRINT_DEBUG("Done");

  BreakpointsCtrl.Attach(Process);

  HandleProcessContinue();
}

void Debugger::HandleProcessStop() {
  BreakpointsCtrl.Detach();
  Process->Detach();
  Process = nullptr;
}

void Debugger::HandleBreakpointSet(
    const std::shared_ptr<PromptCmdBreakpointSet> &BPS) {
  if (BPS->SymbolName) {
    PRINT_DEBUG("SET TO", BPS->SymbolName.Get());
    BreakpointsCtrl.AddBreakpointBySymbolName(BPS->SymbolName.Get(),
                                              HandleSymbolNameBreakpoint_l);
  }
  if (BPS->MethodName) {
    PRINT_DEBUG("SET TO", BPS->MethodName.Get());
  }
}

BreakpointCallbackReturn
Debugger::HandleSymbolNameBreakpoint(std::string SymbolName) {
  PRINT_DEBUG("BREAK ON", SymbolName);
  return BreakpointCallbackReturn::BREAK;
}

int Debugger::Start(int argc, char *argv[]) {
  if (argc < 2) {
    Error Err(MAD_ERROR_ARGUMENTS);
    Err.Log("Expected a programm name as argument");
    return 1;
  }

  Exe = argv[1];
  Prompt.Say("Mini Apple Debugger");
  Prompt.Say("Executable set to", Exe);
  PRINT_DEBUG("PID:", getpid());

  while (true) {
    auto Cmd = Prompt.Show();

    if (!Cmd) {
      Error Err(MAD_ERROR_PROMPT);
      Err.Log("Unknown command...");
      continue;
    }

    switch (Cmd->GetType()) {
    case PromptCmdType::MAD_EXIT:
      return 0;
      break;
    case PromptCmdType::MAD_HELP:
      Prompt.ShowCommands();
      break;

    case PromptCmdType::PROCESS_RUN:
      HandleProcessRun();
      break;

    case PromptCmdType::PROCESS_CONTINUE:
      HandleProcessContinue();
      break;

    case PromptCmdType::BREAKPOINT_SET:
      HandleBreakpointSet(
          std::static_pointer_cast<PromptCmdBreakpointSet>(Cmd));
      break;
    }
  }

  return 0;
}
