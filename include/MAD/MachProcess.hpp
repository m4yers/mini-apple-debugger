#ifndef MACHPROCESS_HPP_DQTLY8NZ
#define MACHPROCESS_HPP_DQTLY8NZ

// #include <CoreFoundation/CoreFoundation.h>

#include <map>
#include <string>
#include <unistd.h>

#include <mach-o/loader.h>
#include <mach/mach.h>
#include <sys/types.h>

#include "MAD/MachTask.hpp"
#include <MAD/Error.hpp>
#include <MAD/MachImage.hpp>

namespace mad {

enum class MachProcessStatusType {
  ERROR,
  CONTINUED,
  STOPPED,
  SIGNALED,
  EXITED
};

class MachProcessStatus {
public:
  MachProcessStatusType Type;
  Error Error;
  int ExitStatus;
  int TermSignal;
  int StopSignal;
};

class MachProcess {

public:
private:
  std::string Exec;
  pid_t PID;
  MachTask Task;
  MachMemory &Memory;
  std::vector<std::shared_ptr<MachImage64>> Images;
  std::map<std::string, std::shared_ptr<MachImage64>> ImagesByName;
  std::map<unsigned, std::vector<std::shared_ptr<MachImage64>>> ImagesByType;

private:
  int RunTarget();
  void FindAllImages();

public:
  MachProcess(std::string exec);
  MachProcess(const MachProcess &) = delete;
  MachProcess &operator=(const MachProcess &) = delete;

  int Execute();
  bool Attach();
  void Detach();
  vm_size_t ReadMemory(vm_address_t address, vm_size_t size, void *data);
  vm_size_t WriteMemory(vm_address_t address, vm_offset_t data,
                        mach_msg_type_number_t count);

  pid_t GetPID() { return PID; }
  bool IsParent() { return PID > 0; }

  auto &GetTask() { return Task; };

  auto &GetImagess() { return Images; }
  auto GetImagesByName(std::string Name) { return ImagesByName[Name]; }
  auto GetImagesByType(unsigned Type) { return ImagesByType[Type]; }

  auto GetDynamicLinkerImage() {
    auto &List = ImagesByType[MH_DYLINKER];
    assert(List.size() == 1);
    return List.front();
  }

  MachProcessStatus Step();
  MachProcessStatus Continue();
  void Wait(MachProcessStatus &);

private:
  using dyld_process_info_create_t = void *(*)(task_t task, uint64_t timestamp,
                                               kern_return_t *kernelError);
  using dyld_process_info_for_each_image_t = void (*)(
      void *info, void (^callback)(uint64_t machHeaderAddress,
                                   const uuid_t uuid, const char *path));
  using dyld_process_info_release_t = void (*)(void *info);
  using dyld_process_info_get_cache_t = void (*)(void *info, void *cacheInfo);

  dyld_process_info_create_t dyld_process_info_create;
  dyld_process_info_for_each_image_t dyld_process_info_for_each_image;
  dyld_process_info_release_t dyld_process_info_release;
  dyld_process_info_get_cache_t dyld_process_info_get_cache;
};
} // namespace mad

#endif /* end of include guard: MACHPROCESS_HPP_DQTLY8NZ */
