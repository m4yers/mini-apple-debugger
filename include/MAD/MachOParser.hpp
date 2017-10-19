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
#include "MAD/Mach.hpp"

namespace mad {

#define MO_PARSE_IMAGE 0
#define MO_PARSE_FILE 1

template <typename T, typename = IsMachSystem_t<T>> class MachOParser {
private:
  using HeaderCmd_t = std::conditional_t<std::is_same_v<T, MachSystem32_t>,
                                         mach_header, mach_header_64>;
  using SegmentCmd_t = std::conditional_t<std::is_same_v<T, MachSystem32_t>,
                                          segment_command, segment_command_64>;
  using SectionCmd_t = std::conditional_t<std::is_same_v<T, MachSystem32_t>,
                                          section, section_64>;
  using NList_t = std::conditional_t<std::is_same_v<T, MachSystem32_t>,
                                     struct nlist, struct nlist_64>;

  static const bool Is32 = std::is_same_v<T, MachSystem32_t>;
  static const bool Is64 = std::is_same_v<T, MachSystem64_t>;
  static const uint32_t lc_segment = Is32 ? LC_SEGMENT : LC_SEGMENT_64;

private:
  template <typename S>
  static bool ReadAThingFromInput(std::istream &Input,
                                  std::shared_ptr<S> &Thing) {
    Thing = std::make_shared<S>();
    if (Input.read((char *)&Thing->Raw, sizeof(Thing->Raw))) {
      Thing->Parse(Input);
      return true;
    }
    return false;
  }

  template <typename S>
  static bool
  ReadAThingFromInputAndPush(std::istream &Input,
                             std::vector<std::shared_ptr<S>> &Container) {
    std::shared_ptr<S> Thing = std::make_shared<S>();
    if (Input.read((char *)&Thing->Raw, sizeof(Thing->Raw))) {
      Thing->Parse(Input);
      Container.push_back(std::move(Thing));
      return true;
    }
    return false;
  }

  static std::string ReadNTStringFromInput(std::istream &Input) {
    std::string result;
    while (Input.good()) {
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
  template <typename R> class MachOThing {
  public:
    R Raw;
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
    std::vector<std::shared_ptr<MachOSection>> Sections;

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
        ReadAThingFromInputAndPush(Input, Sections);
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
      auto Index = Raw.n_un.n_strx;

      // For whatever reason symbol table may contain invalid records with
      // n_strx pointing way beyond its string table limits. Dynamic Loader
      // just skipts those, so does this parser.
      if (Index > Parser.SymbolTable->StringTableSize) {
        return true;
      }

      // Zero string table offsets means there is no name for the thing
      if (Index) {
        // At this point we a sure __LINKEDIT is present
        Input.seekg(Parser.SymbolTable->StringTableOffset + Index);
        Name = ReadNTStringFromInput(Input);
      }

      return true;
    }
  };

  class MachOSymbolTable : public MachOThing<symtab_command> {
  public:
    using MachOThing<symtab_command>::Raw;
    uint32_t StringTableOffset;
    uint32_t StringTableSize;
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
        StringTableSize = Raw.strsize;

        Input.seekg(seg->ActualAddress + Raw.symoff - seg->FileOffset);
        for (uint32_t i = 0; i < Raw.nsyms && Input.good(); ++i) {
          ReadAThingFromInputAndPush(Input, Symbols);
        }

        for (auto symbol : Symbols) {
          symbol->PostParse(Parser);
          if (!Input.good()) {
            return false;
          }
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
  uint32_t Flags;

public:
  std::shared_ptr<MachOHeader> Header;
  std::vector<std::shared_ptr<MachOSegment>> Segments;
  std::shared_ptr<MachOSymbolTable> SymbolTable;
  std::shared_ptr<MachODySymbolTable> DySymbolTable;
  std::vector<std::shared_ptr<MachODyLibrary>> DyLibraries;
  std::shared_ptr<MachODyLibrary> DyLibraryId;
  std::shared_ptr<MachODyLinker> DyLinker;
  std::shared_ptr<MachODyLinker> DyLinkerId;

public:
  MachOParser(std::string Label, std::istream &Input, uint32_t Flags)
      : Label(Label), Input(Input), Flags(Flags) {}

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
    ReadAThingFromInput(Input, Header);
    mainptr += sizeof(mach_header_64);
    PRINT_DEBUG("HEADER magic: ", HEX(Header->Raw.magic),
                ", ncmds: ", Header->Raw.ncmds);

    for (uint32_t i = 0; i < Header->Raw.ncmds; ++i) {
      load_command loadcmd;
      Input.seekg(mainptr);
      Input.read((char *)&loadcmd, sizeof(load_command));
      Input.seekg(mainptr);

      switch (loadcmd.cmd) {

      case lc_segment: {
        ReadAThingFromInputAndPush(Input, Segments);
        break;
      }

      case LC_SYMTAB: {
        ReadAThingFromInput(Input, SymbolTable);
        break;
      }

      case LC_DYSYMTAB: {
        ReadAThingFromInput(Input, DySymbolTable);
        break;
      }

      case LC_ID_DYLIB: {
        ReadAThingFromInput(Input, DyLibraryId);
        break;
      }

      case LC_LOAD_DYLIB:
      case LC_LOAD_WEAK_DYLIB:
      case LC_REEXPORT_DYLIB: {
        ReadAThingFromInputAndPush(Input, DyLibraries);
        break;
      }

      case LC_ID_DYLINKER: {
        ReadAThingFromInput(Input, DyLinkerId);
        break;
      }

      case LC_LOAD_DYLINKER: {
        ReadAThingFromInput(Input, DyLinker);
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

private:
  bool PostParse() {
    if (SymbolTable) {
      SymbolTable->PostParse(*this);
    }
    return true;
  }
};

using MachOParser32 = MachOParser<MachSystem32_t>;
using MachOParser64 = MachOParser<MachSystem64_t>;

} // namespace mad

#endif /* end of include guard: MACHOPARSER_HPP_L6XJ5WJN */
