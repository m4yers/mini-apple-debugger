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
  }
  PRINT_DEBUG("Attached...");

  uintptr_t address = 0x100003000;
  uint64_t data = 10;
  unsigned long size;

  unsigned icounter = 0;
  while (WIFSTOPPED(wait_status)) {
    icounter++;

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
    //
    // if (Task.Read(address, 8, &data, &size)) {
    //   PRINT_DEBUG("READ FROM ADDRESS: ", (void *)address,
    //       ", DATA: ", (void *)data);
    //   break;
    // }
    //
    // if (ptrace(PT_STEP, Process.GetPID(), (caddr_t)1, 0) < 0) {
    //   PRINT_ERROR(std::strerror(errno));
    //   return;
    // }

    break;
    wait(&wait_status);
  }

  PRINT_DEBUG("Executed ", icounter, " instructions");
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
