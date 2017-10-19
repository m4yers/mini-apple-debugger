#ifndef MACHOPARSER_HPP_L6XJ5WJN
#define MACHOPARSER_HPP_L6XJ5WJN

#include <cassert>
#include <istream>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <unistd.h>
#include <vector>

#include <dlfcn.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
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
  using NList_t = std::conditional_t<std::is_same_v<T, MachOParser32_t>,
                                     struct nlist, struct nlist_64>;

#define READ_IMAGE(type, name)                                                 \
  auto name##cmd = &name.Raw;                                                  \
  Input.read((char *)name##cmd, sizeof(name.Raw));                             \
  name.Parse(Input);

#define READ_IMAGE_L(type, name)                                               \
  type name;                                                                   \
  auto name##cmd = &name.Raw;                                                  \
  Input.read((char *)name##cmd, sizeof(name.Raw));                             \
  name.Parse(Input);

#define READ_IMAGE_L_S(type, name)                                             \
  auto name = std::make_shared<type>();                                        \
  auto name##cmd = &name->Raw;                                                 \
  Input.read((char *)name##cmd, sizeof(name->Raw));                            \
  name->Parse(Input);

  static const bool Is32 = std::is_same_v<T, MachOParser32_t>;
  static const bool Is64 = std::is_same_v<T, MachOParser64_t>;
  static const uint32_t lc_segment = Is32 ? LC_SEGMENT : LC_SEGMENT_64;

private:
  static std::string ReadNTStringFromInput(std::istream &Input) {
    std::string result;
    while (true) {
      char ch;
      Input.read(&ch, 1);
      if (!ch) {
        return result;
      }
      result.push_back(ch);
    }
    return result;
  }

public:
  template <typename C> class MachOThing {
  public:
    C Raw;
    bool Parse(std::istream &Input) { return true; }
    bool PostParse(MachOParser &Parser) { return true; }
  };

  class MachOHeader : public MachOThing<HeaderCmd_t> {};

  class MachOSection : public MachOThing<SectionCmd_t> {
  public:
    using MachOThing<SectionCmd_t>::Raw;
    std::string Name;
    std::string SegmentName;

  public:
    bool Parse(std::istream &Input) {
      Name = std::string(Raw.sectname);
      SegmentName = std::string(Raw.segname);
      return true;
    }
  };

  class MachOSegment : public MachOThing<SegmentCmd_t> {
  public:
    using MachOThing<SegmentCmd_t>::Raw;
    std::string Name;
    uint32_t VirtualAddress;
    uint32_t VirtualSize;
    uint32_t FileOffset;
    uint32_t FileSize;
    // The actual address of this segment within an entity we are currently
    // parsing. The variable will contain virtual address if we parse an image
    // or file offset if we parse an object file.
    uint32_t ActualAddress;
    std::vector<MachOSection> Sections;

  public:
    bool Parse(std::istream &Input) {
      Name = std::string(Raw.segname);
      VirtualAddress = Raw.vmaddr;
      VirtualSize = Raw.vmsize;
      FileOffset = Raw.fileoff;
      FileSize = Raw.filesize;

      // TODO Set this depending of flags to MachOParser::Parse
      ActualAddress = VirtualAddress;

      for (uint32_t s = 0; s < Raw.nsects; ++s) {
        READ_IMAGE_L(MachOSection, section);
        Sections.push_back(std::move(section));
      }

      return true;
    }
  };

  class MachOSymbolTableEntry : public MachOThing<NList_t> {
  public:
    using MachOThing<NList_t>::Raw;
    std::string Name;

  public:
    // Do string retrieval in post-parse call so we would not jump memory
    // between symbols and strings often
    bool PostParse(MachOParser &Parser) {
      auto &Input = Parser.Input;
      if (auto index = Raw.n_un.n_strx) {
        // At this moment we a sure __LINKEDIT is present
        Input.seekg(Parser.SymbolTable.StringTableOffset + index);
        Name = ReadNTStringFromInput(Input);
      }
      return true;
    }
  };

  class MachOSymbolTable : public MachOThing<symtab_command> {
  public:
    using MachOThing<symtab_command>::Raw;
    uint32_t StringTableOffset;
    std::vector<std::shared_ptr<MachOSymbolTableEntry>> Symbols;

  public:
    bool PostParse(MachOParser &Parser) {
      // We know that symbol table records reside in __LINKEDIT segment, but
      // the symoff is given relative to the object file and not segment
      // itself. We have to subtract segment fileoff from this value to get
      // segment relative offset.
      if (auto seg = Parser.GetSegmentByName(SEG_LINKEDIT)) {
        auto &Input = Parser.Input;
        StringTableOffset = seg->ActualAddress + Raw.stroff - seg->FileOffset;
        Input.seekg(seg->ActualAddress + Raw.symoff - seg->FileOffset);
        for (uint32_t i = 0; i < Raw.nsyms; ++i) {
          READ_IMAGE_L_S(MachOSymbolTableEntry, symbol);
          Symbols.push_back(std::move(symbol));
        }

        for (auto symbol : Symbols) {
          symbol->PostParse(Parser);
        }

        return true;
      }
      return false;
    }
  };

  class MachODySymbolTable : public MachOThing<dysymtab_command> {};

  class MachODyLibrary : public MachOThing<dylib_command> {
  public:
    std::string Name;

  public:
    using MachOThing<dylib_command>::Raw;
    bool Parse(std::istream &Input) {
      Input.seekg((size_t)Input.tellg() + Raw.dylib.name.offset - sizeof(Raw));
      Name = ReadNTStringFromInput(Input);
      return true;
    }
  };

  class MachODyLinker : public MachOThing<dylinker_command> {
  public:
    std::string Name;

  public:
    using MachOThing<dylinker_command>::Raw;
    bool Parse(std::istream &Input) {
      Input.seekg((size_t)Input.tellg() + Raw.name.offset - sizeof(Raw));
      Name = ReadNTStringFromInput(Input);
      return true;
    }
  };

private:
  std::string Label;
  std::istream &Input;

public:
  MachOHeader Header;
  std::vector<std::shared_ptr<MachOSegment>> Segments;
  MachOSymbolTable SymbolTable;
  MachODySymbolTable DySymbolTable;
  std::vector<std::shared_ptr<MachODyLibrary>> DyLibraries;
  MachODyLibrary DyLibraryId;
  MachODyLinker DyLinker;
  MachODyLinker DyLinkerId;

public:
  MachOParser(std::string label, std::istream &input)
      : Label(label), Input(input) {}

  std::shared_ptr<MachOSegment> GetSegmentByName(std::string name) {
    for (auto segment : Segments) {
      if (segment->Name == name) {
        return segment;
      }
    }
    return nullptr;
  }

  bool Parse() {
    PRINT_DEBUG("PARSING ", Label, " ...");

    uint64_t mainptr = 0;
    Input.seekg(mainptr);
    Input.read((char *)&Header.Raw, sizeof(HeaderCmd_t));
    Header.Parse(Input);
    mainptr += sizeof(mach_header_64);
    PRINT_DEBUG("HEADER magic: ", HEX(Header.Raw.magic),
                ", ncmds: ", Header.Raw.ncmds);

    for (uint32_t i = 0; i < Header.Raw.ncmds; ++i) {
      load_command loadcmd;
      Input.seekg(mainptr);
      Input.read((char *)&loadcmd, sizeof(load_command));
      Input.seekg(mainptr);

      switch (loadcmd.cmd) {

      case lc_segment: {
        READ_IMAGE_L_S(MachOSegment, segment);
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
        READ_IMAGE_L(MachODyLibrary, DyLibraryId);
        break;
      }

      case LC_LOAD_DYLIB:
      case LC_LOAD_WEAK_DYLIB:
      case LC_REEXPORT_DYLIB: {
        READ_IMAGE_L_S(MachODyLibrary, dylibrary);
        DyLibraries.push_back(std::move(dylibrary));
        break;
      }

      case LC_ID_DYLINKER: {
        READ_IMAGE_L(MachODyLinker, DyLinkerId);
        break;
      }

      case LC_LOAD_DYLINKER: {
        READ_IMAGE_L(MachODyLinker, DyLinker);
        break;
      }
      default: {
        PRINT_DEBUG("UNKNOWN LOAD COMMAND ", loadcmd.cmd);
        break;
      }
      }

      if (!Input.good()) {
        PRINT_ERROR("Failed to parse ", Label, " at ", mainptr);
        return false;
      }

      mainptr += loadcmd.cmdsize;
    }

    return PostParse();
  }

  bool PostParse() {
    SymbolTable.PostParse(*this);
    return true;
  }
};

using MachOParser32 = MachOParser<MachOParser32_t>;
using MachOParser64 = MachOParser<MachOParser64_t>;

} // namespace mad

#endif /* end of include guard: MACHOPARSER_HPP_L6XJ5WJN */
