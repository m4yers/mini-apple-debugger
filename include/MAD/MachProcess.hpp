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

namespace mad {
class MachProcess {

public:
  // These structures somewhat duplicate existing ones from <mach-o/loader.h>
  // but do that for convenence sake.
  struct mach_o_segment {
    std::string name;
    uint64_t vmaddr;
    uint64_t vmsize;
    uint64_t fileoff;
    uint64_t filesize;
    uint64_t maxprot;
    uint64_t initprot;
    uint64_t nsects;
    uint64_t flags;
  };
  struct mach_o_information {
    struct mach_header_64 mach_header;
    std::vector<struct mach_o_segment> segments;
    uuid_t uuid;
    std::string min_version_os_name;
    std::string min_version_os_version;
  };
  struct binary_image_information {
    std::string path;
    uint64_t load_address;
    struct mach_o_information macho_info;
  };

private:
  std::string Exec;
  pid_t PID;
  MachTask Task;
  std::map<std::string, binary_image_information> BinaryImages;

private:
  int RunTarget();
  void FindAllImages();

public:
  MachProcess(std::string exec);
  int Execute();
  bool Attach();
  vm_size_t ReadMemory(vm_address_t address, vm_size_t size, void *data);
  vm_size_t WriteMemory(vm_address_t address, vm_offset_t data,
                        mach_msg_type_number_t count);

  pid_t GetPID() { return PID; }
  bool IsParent() { return PID > 0; }

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
