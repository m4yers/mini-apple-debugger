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

void Debugger::HandleContinue() {
  if (!Process) {
    Prompt.Say("You must run the program first");
    return;
  }

  bool Continue = true;
  while (Continue) {
    Continue = false;
    auto Status = Process->Continue();
    switch (Status.Type) {
    case MachProcessStatusType::ERROR:
      Prompt.Say("Could not run program", Exe);
      break;
    case MachProcessStatusType::CONTINUED:
      break;
    case MachProcessStatusType::EXITED:
      Prompt.Say("Program", Exe, "finished with status", Status.ExitStatus);
      BreakpointsCtrl.release();
      Process->Detach();
      Process.release();
      break;
    case MachProcessStatusType::SIGNALED:
      Prompt.Say("Program", Exe, "signaled");
      break;
    case MachProcessStatusType::STOPPED:
      // FIXME handle signals
      PRINT_DEBUG("SIGNAL: ", Status.StopSignal);
      Continue = BreakpointsCtrl->CheckBreakpoints();
      break;
    }
  }
}
void Debugger::HandleRun() {
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

  int WaitStatus;
  waitpid(Process->GetPID(), &WaitStatus, 0);

  PRINT_DEBUG("Attaching to", Exe, "at", Process->GetPID(), "...");
  if (!Process->Attach()) {
    PRINT_ERROR("Failed to attach");
    exit(1);
  }
  PRINT_DEBUG("Done");

  BreakpointsCtrl = std::make_unique<Breakpoints>(*Process.get());
  BreakpointsCtrl->PreRun();

  HandleContinue();
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
      Err.Log("Unknow command...");
      continue;
    }

    switch (Cmd->GetType()) {
    case PromptCmdType::EXIT:
      return 0;
      break;
    case PromptCmdType::HELP:
      Prompt.ShowHelp();
      break;

    case PromptCmdType::RUN:
      HandleRun();
      break;

    case PromptCmdType::CONTINUE:
      HandleContinue();
      break;
    }
  }

  return 0;
}
