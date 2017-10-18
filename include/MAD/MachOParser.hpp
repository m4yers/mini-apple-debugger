#ifndef MACHOPARSER_HPP_L6XJ5WJN
#define MACHOPARSER_HPP_L6XJ5WJN

#include <cassert>
#include <istream>
#include <map>
#include <string>
#include <type_traits>
#include <unistd.h>
#include <vector>

#include <dlfcn.h>
#include <mach-o/loader.h>
#include <uuid/uuid.h>

#include "MAD/Debug.hpp"

namespace mad {

using MachOParser32_t = std::integral_constant<int, 1>;
using MachOParser64_t = std::integral_constant<int, 0>;

template <typename T,
          typename = std::enable_if_t<std::is_same_v<T, MachOParser32_t> ||
                                      std::is_same_v<T, MachOParser64_t>>>
class MachOParser {
private:
  using HeaderCmd_t = std::conditional_t<std::is_same_v<T, MachOParser32_t>,
                                         mach_header, mach_header_64>;
  using SegmentCmd_t = std::conditional_t<std::is_same_v<T, MachOParser32_t>,
                                          segment_command, segment_command_64>;
  using SectionCmd_t = std::conditional_t<std::is_same_v<T, MachOParser32_t>,
                                          section, section_64>;

  static const bool Is32 = std::is_same_v<T, MachOParser32_t>;
  static const bool Is64 = std::is_same_v<T, MachOParser64_t>;
  static const uint32_t lc_segment = Is32 ? LC_SEGMENT : LC_SEGMENT_64;

private:
  static std::string ReadLCStringFromInput(std::istream &input) {
    std::string result;
    while (true) {
      char ch;
      input.read(&ch, 1);
      if (!ch) {
        return result;
      }
      result.push_back(ch);
    }
    return result;
  }

public:
  class MachOHeader {
    friend MachOParser;
    HeaderCmd_t Header;

  private:
    MachOHeader() {}
    bool Init(std::istream &input) { return true; }

  public:
  };

  class MachOSection {
    friend MachOParser;
    SectionCmd_t Command;

  private:
    MachOSection() = default;
    bool Init(std::istream &input) {
      Name = std::string(Command.sectname, sizeof(Command.sectname));
      SegmentName = std::string(Command.segname, sizeof(Command.segname));
      return true;
    }

  public:
    std::string Name;
    std::string SegmentName;
  };

  class MachOSegment {
    friend MachOParser;
    SegmentCmd_t Command;

  private:
    MachOSegment() = default;
    bool Init(std::istream &input) {
      Name = std::string(Command.segname, sizeof(Command.segname));
      return true;
    }

  public:
    std::string Name;
    std::vector<MachOSection> Sections;
  };

  class MachOSymbolTable {
    friend MachOParser;
    symtab_command Command;

  private:
    MachOSymbolTable() = default;
    bool Init(std::istream &input) { return true; }
  };

  class MachODySymbolTable {
    friend MachOParser;
    dysymtab_command Command;

  private:
    MachODySymbolTable() = default;
    bool Init(std::istream &input) { return true; }
  };

  class MachODyLibrary {
    friend MachOParser;
    dylib_command Command;

  private:
    MachODyLibrary() = default;
    bool Init(std::istream &input) {
      input.seekg((size_t)input.tellg() + Command.dylib.name.offset -
                  sizeof(Command));
      Name = ReadLCStringFromInput(input);
      return true;
    }

  public:
    std::string Name;
  };

  class MachODyLinker {
    friend MachOParser;
    dylinker_command Command;

  private:
    MachODyLinker() = default;
    bool Init(std::istream &input) {
      input.seekg((size_t)input.tellg() + Command.name.offset -
                  sizeof(Command));
      Name = ReadLCStringFromInput(input);
      return true;
    }

  public:
    std::string Name;
  };

private:
  std::string Label;
  std::istream &Input;

public:
  MachOHeader Header;
  std::vector<MachOSegment> Segments;
  MachOSymbolTable SymbolTable;
  MachODySymbolTable DySymbolTable;
  std::vector<MachODyLibrary> DyLibraries;
  MachODyLibrary DyId;
  MachODyLinker DyLinker;

#define READ_IMAGE(type, name)                                                 \
  auto name##cmd = &name.Command;                                              \
  Input.read((char *)name##cmd, sizeof(name.Command));                         \
  name.Init(this->Input);

#define READ_IMAGE_L(type, name)                                               \
  type name;                                                                   \
  auto name##cmd = &name.Command;                                              \
  Input.read((char *)name##cmd, sizeof(name.Command));                         \
  name.Init(this->Input);

public:
  MachOParser(std::string label, std::istream &input)
      : Label(label), Input(input) {}
  bool Parse() {
    PRINT_DEBUG("PARSING ", Label, " ...");

    uint64_t mainptr = 0;
    Input.seekg(mainptr);
    Input.read((char *)&Header.Header, sizeof(HeaderCmd_t));
    mainptr += sizeof(mach_header_64);
    PRINT_DEBUG("HEADER magic: ", HEX(Header.Header.magic),
                ", ncmds: ", Header.Header.ncmds);

    for (uint32_t i = 0; i < Header.Header.ncmds; ++i) {
      load_command loadcmd;
      Input.seekg(mainptr);
      Input.read((char *)&loadcmd, sizeof(load_command));
      Input.seekg(mainptr);

      switch (loadcmd.cmd) {

      case lc_segment: {
        READ_IMAGE_L(MachOSegment, segment);

        uint64_t secptr = mainptr + sizeof(SegmentCmd_t);

        for (uint32_t s = 0; s < segmentcmd->nsects; ++s) {
          Input.seekg(secptr + s * sizeof(SectionCmd_t));
          READ_IMAGE_L(MachOSection, section);
          segment.Sections.push_back(std::move(section));
        }

        Segments.push_back(std::move(segment));
        break;
      }

      case LC_SYMTAB: {
        READ_IMAGE(MachOSymbolTable, SymbolTable);
        break;
      }

      case LC_DYSYMTAB: {
        READ_IMAGE(MachODySymbolTable, DySymbolTable);
        break;
      }

      case LC_ID_DYLIB: {
        READ_IMAGE_L(MachODyLibrary, DyId);
        break;
      }

      case LC_LOAD_DYLIB:
      case LC_LOAD_WEAK_DYLIB:
      case LC_REEXPORT_DYLIB: {
        READ_IMAGE_L(MachODyLibrary, dylibrary);
        DyLibraries.push_back(std::move(dylibrary));
        break;
      }

      case LC_LOAD_DYLINKER: {
        READ_IMAGE_L(MachODyLinker, DyLinker);
        break;
      }
      }

      if (!Input.good()) {
        PRINT_ERROR("Failed to parse ", Label, " at ", mainptr);
        return false;
      }

      mainptr += loadcmd.cmdsize;
    }

    return true;
  }
};

using MachOParser32 = MachOParser<MachOParser32_t>;
using MachOParser64 = MachOParser<MachOParser64_t>;

} // namespace mad

#endif /* end of include guard: MACHOPARSER_HPP_L6XJ5WJN */
