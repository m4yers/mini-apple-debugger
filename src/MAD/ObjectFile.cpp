#include "MAD/Debug.hpp"
#include "MAD/ObjectFile.hpp"
#include <MAD/MachOParser.hpp>

using namespace mad;

bool ObjectFile::Parse() {
  PRINT_DEBUG("Parsing MachO data from ", Path);

  uint64_t loadptr = 0;
  Input.seekg(loadptr);
  Input.read((char *)&Header, sizeof(mach_header_64));
  loadptr += sizeof(mach_header_64);

  // Remove capability bits
  Header.cpusubtype &= ~CPU_SUBTYPE_MASK;

  for (uint32_t i = 0; i < Header.ncmds; ++i) {
    load_command loadcmd;
    Input.seekg(loadptr);
    Input.read((char *)&loadcmd, sizeof(load_command));
    Input.seekg(loadptr);

    if (loadcmd.cmd == LC_SEGMENT_64) {
      segment_command_64 segcmd;
      Input.read((char *)&segcmd, sizeof(segment_command_64));
      Segment segment;
      segment.name = std::string(segcmd.segname, 16);
      segment.vmaddr = segcmd.vmaddr;
      segment.vmsize = segcmd.vmsize;
      segment.fileoff = segcmd.fileoff;
      segment.filesize = segcmd.filesize;
      segment.maxprot = segcmd.maxprot;
      segment.initprot = segcmd.initprot;
      segment.nsects = segcmd.nsects;
      segment.flags = segcmd.flags;
      Segments.push_back(segment);
      PRINT_DEBUG("SEG: ", segment.name, ", NSECTS: ", segment.nsects);
    }

    if (loadcmd.cmd == LC_UUID) {
      uuid_command uuidcmd;
      Input.read((char *)&uuidcmd, sizeof(uuid_command));
      uuid_copy(UUID, uuidcmd.uuid);
    }

    bool loadcmd_known = loadcmd.cmd == LC_VERSION_MIN_IPHONEOS ||
                         loadcmd.cmd == LC_VERSION_MIN_MACOSX;
    if (loadcmd_known) {
      version_min_command vercmd;
      Input.read((char *)&vercmd, sizeof(version_min_command));
      switch (loadcmd.cmd) {
      case LC_VERSION_MIN_IPHONEOS:
        MinVersionOsName = "iphoneos";
        break;
      case LC_VERSION_MIN_MACOSX:
        MinVersionOsName = "macosx";
        break;
      default:
        return false;
      }
      uint32_t xxxx = vercmd.sdk >> 16;
      uint32_t yy = (vercmd.sdk >> 8) & 0xffu;
      uint32_t zz = vercmd.sdk & 0xffu;
      MinVersionOsVersion = "";
      MinVersionOsVersion += std::to_string(xxxx);
      MinVersionOsVersion += ".";
      MinVersionOsVersion += std::to_string(yy);
      if (zz != 0) {
        MinVersionOsVersion += ".";
        MinVersionOsVersion += std::to_string(zz);
      }
    }

    loadptr += loadcmd.cmdsize;
  }

  return true;
}
