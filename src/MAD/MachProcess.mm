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

// Objc
// #import <Foundation/Foundation.h>

// MAD
#include "MAD/Debug.hpp"
#include "MAD/MachOParser.hpp"
#include "MAD/MachProcess.hpp"
#include "MAD/MachTaskMemoryStream.hpp"
#include "MAD/ObjectFile.hpp"
#include "MAD/UniversalBinary.hpp"

using namespace mad;

typedef void *dyld_process_info;

MachProcess::MachProcess(std::string exec) : Exec(exec), PID(0) {
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
  dyld_process_info_for_each_image(
      info, ^(uint64_t mach_header_addr, const uuid_t uuid, const char *path) {
      PRINT_DEBUG("PATH: ", path);

      MachTaskMemoryStreamBuf buffer(Task, mach_header_addr);
      std::istream stream(&buffer);
      MachOParser64 parser(path, stream);
      parser.Parse();

      struct binary_image_information image;
      image.path = path;
      image.load_address = mach_header_addr;
      uuid_copy(image.macho_info.uuid, uuid);
      BinaryImages.insert({path, image});
      });
  dyld_process_info_release(info);
}

#define READ_MACHO_MEMORY(offset, type, variable)                              \
  type variable;                                                               \
  if (ReadMemory(offset, sizeof(type), &variable) != sizeof(type)) {           \
    PRINT_ERROR("could not read struct #type#");                               \
    return false;                                                              \
  }

// TODO: This method must not return a memory read containing breakpoints. It
// will clean the read buffer from enabled breakpoints
vm_size_t MachProcess::ReadMemory(vm_address_t address, vm_size_t size,
    void *data) {
  vm_size_t bytes = Task.ReadMemory(address, size, data);
  return bytes;
}

// TODO: This method must not overwrite existing breakpoints. It will write
// memory first then re apply enabled breakpoints
vm_size_t MachProcess::WriteMemory(vm_address_t address, vm_offset_t data,
    mach_msg_type_number_t count) {
  vm_size_t bytes = Task.WriteMemory(address, data, count);
  return bytes;
}

int MachProcess::RunTarget() {
  PRINT_DEBUG("Target started. Will run ", Exec);

  if (ptrace(PT_TRACE_ME, 0, 0, 0) < 0) {
    PRINT_ERROR("ptrace me");
    return 2;
  }

  execl(Exec.c_str(), Exec.c_str(), NULL);
  PRINT_ERROR("execl");
  return 2;
}

int MachProcess::Execute() {
  PID = fork();
  if (PID == 0) {
    return RunTarget();
  } else if (PID < 0) {
    PRINT_ERROR("Fork");
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
