// System
#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <mach-o/fat.h>
#include <mach/mach.h>
#include <mach/machine.h>
#include <sys/types.h>
#include <uuid/uuid.h>

#include <sys/ptrace.h>

// Std
#include <cassert>
#include <fstream>
#include <istream>
#include <memory>

// Objc
// #import <Foundation/Foundation.h>

// MAD
#include "MAD/Error.hpp"
#include "MAD/MachImage.hpp"
#include "MAD/MachOParser.hpp"
#include "MAD/MachProcess.hpp"
#include "MAD/MachTaskMemoryStream.hpp"
#include "MAD/ObjectFile.hpp"
#include "MAD/UniversalBinary.hpp"

using namespace mad;

typedef void *dyld_process_info;

MachProcess::MachProcess(std::string exec)
    : Exec(exec), PID(0), Task(), Memory(Task.GetMemory()) {
  dyld_process_info_create = (dyld_process_info_create_t)dlsym(
      RTLD_DEFAULT, "_dyld_process_info_create");
  dyld_process_info_for_each_image = (dyld_process_info_for_each_image_t)dlsym(
      RTLD_DEFAULT, "_dyld_process_info_for_each_image");
  dyld_process_info_release = (dyld_process_info_release_t)dlsym(
      RTLD_DEFAULT, "_dyld_process_info_release");
  dyld_process_info_get_cache = (dyld_process_info_get_cache_t)dlsym(
      RTLD_DEFAULT, "_dyld_process_info_get_cache");
}

void MachProcess::FindAllImages() {
  kern_return_t kern_ret;
  dyld_process_info info =
      dyld_process_info_create(Task.GetPort(), 0, &kern_ret);
  dyld_process_info_for_each_image(info, ^(uint64_t mach_header_addr,
                                           const uuid_t, const char *path) {
    // SIDE NOTE: Why the fuck I cannot use move-constructor here?
    auto Image = std::make_shared<MachImage64>(path, Task, mach_header_addr);
    Image->Scan();
    ImagesByName.insert({path, Image});
    auto Type = Image->GetType();
    if (!ImagesByType.count(Type)) {
      ImagesByType.insert({Type, {}});
    }
    ImagesByType[Type].push_back(Image);
    Images.push_back(Image);

  });
  dyld_process_info_release(info);
  // for (auto &pair : ImagesByName) {
  //   auto &Image = pair.second;
  //   auto &SymbolTable = Image->GetSymbolTable();
  //   for (auto Entry : SymbolTable.GetSymbols()) {
  //     auto Section = Entry->Raw.n_sect ?
  //     Image->GetSectionByIndex(Entry->Raw.n_sect) : nullptr;
  //     PRINT_DEBUG("SYMBOL type: ", HEX(Entry->Raw.n_type),
  //                      ", sect: ", (Section ? Section->Name : "-"),
  //                      ", desc: ", HEX(Entry->Raw.n_desc),
  //                     ", value: ", HEX(Entry->Raw.n_value),
  //                             " ", Entry->Name);
  //   }
  // }
}

// TODO: ACTUALLY... use stream buffer here
// TODO: This method must not return a memory read containing breakpoints. It
// will clean the read buffer from enabled breakpoints
vm_size_t MachProcess::ReadMemory(vm_address_t address, vm_size_t size,
                                  void *data) {
  vm_size_t bytes = Memory.Read(address, size, data);
  return bytes;
}

// TODO: This method must not overwrite existing breakpoints. It will write
// memory first then re apply enabled breakpoints
vm_size_t MachProcess::WriteMemory(vm_address_t address, vm_offset_t data,
                                   mach_msg_type_number_t count) {
  vm_size_t bytes = Memory.Write(address, data, count);
  return bytes;
}

int MachProcess::RunTarget() {
  PRINT_DEBUG("Target started. Will run ", Exec);

  if (ptrace(PT_TRACE_ME, 0, 0, 0) < 0) {
    Error::FromErrno().Log("Could not PT_TRACE_ME");
    return 2;
  }

  execl(Exec.c_str(), Exec.c_str(), NULL);
  Error::FromErrno().Log("Could not execl");
  return 2;
}

int MachProcess::Execute() {
  PID = fork();
  if (PID == 0) {
    return RunTarget();
  } else if (PID < 0) {
    return 1;
  }

  return 0;
}

bool MachProcess::Attach() {
  assert(PID);
  Task.Attach(PID);
  FindAllImages();
  return true;
}

void MachProcess::Wait(MachProcessStatus &Status) {
  int WaitStatus;
  wait(&WaitStatus);
  if (WIFCONTINUED(WaitStatus)) {
    Status.Type = MachProcessStatusType::CONTINUED;
  } else if (WIFSTOPPED(WaitStatus)) {
    Status.Type = MachProcessStatusType::STOPPED;
  } else if (WIFSIGNALED(WaitStatus)) {
    Status.Type = MachProcessStatusType::SIGNALED;
  } else if (WIFEXITED(WaitStatus)) {
    Status.Type = MachProcessStatusType::EXITED;
  }

  Status.ExitStatus = WEXITSTATUS(WaitStatus);
  Status.TermSignal = WTERMSIG(WaitStatus);
  Status.StopSignal = WSTOPSIG(WaitStatus);
}

MachProcessStatus MachProcess::Step() {
  MachProcessStatus Status;

  if (ptrace(PT_STEP, PID, (caddr_t)1, 0) < 0) {
    Status.Type = MachProcessStatusType::ERROR;
    Status.Error = Error(MAD_ERROR_PROCESS);
    Status.Error.Log("Counld not step process", PID);
  } else {
    Wait(Status);
  }

  return Status;
}

MachProcessStatus MachProcess::Continue() {
  MachProcessStatus Status;

  if (ptrace(PT_CONTINUE, PID, (caddr_t)1, 0) < 0) {
    Status.Type = MachProcessStatusType::ERROR;
    Status.Error = Error(MAD_ERROR_PROCESS);
    Status.Error.Log("Counld not continue process", PID);
  } else {
    Wait(Status);
  }

  return Status;
}
