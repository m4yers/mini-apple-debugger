#ifndef OBJECTFILE_HPP_VR37Q6CT
#define OBJECTFILE_HPP_VR37Q6CT

#include <mach-o/loader.h>
#include <mach/mach.h>
#include <uuid/uuid.h>

#include <fstream>
#include <string>
#include <vector>

namespace mad {
class ObjectFile {
  std::string Path;
  std::ifstream &Input;

private:
  struct Segment {
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

private:
  mach_header_64 Header;
  uuid_t UUID;
  std::string MinVersionOsName;
  std::string MinVersionOsVersion;
  std::vector<Segment> Segments;

public:
  ObjectFile(std::string path, std::ifstream &input)
      : Path(path), Input(input) {}
  bool Parse();
};
} // namespace mad

#endif /* end of include guard: OBJECTFILE_HPP_VR37Q6CT */
