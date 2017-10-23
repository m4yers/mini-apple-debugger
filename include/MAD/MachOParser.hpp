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
#include <mach-o/stab.h>
#include <uuid/uuid.h>

#include "MAD/Error.hpp"
#include "MAD/Mach.hpp"
#include "MAD/Utils.hpp"

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

  class MachOHeader : public MachOThing<HeaderCmd_t> {
  public:
    using MachOThing<HeaderCmd_t>::Raw;
    uint32_t Filetype = 0;

    // File type
    BoundFlagEq<MH_OBJECT> IsTypeObject{Filetype};
    BoundFlagEq<MH_EXECUTE> IsTypeExecute{Filetype};
    BoundFlagEq<MH_FVMLIB> IsTypeFVMLibrary{Filetype};
    BoundFlagEq<MH_CORE> IsTypeCore{Filetype};
    BoundFlagEq<MH_PRELOAD> IsTypePreload{Filetype};
    BoundFlagEq<MH_DYLIB> IsTypeDynamicLibrary{Filetype};
    BoundFlagEq<MH_DYLINKER> IsTypeDynamicLinker{Filetype};
    BoundFlagEq<MH_BUNDLE> IsTypeBundle{Filetype};
    BoundFlagEq<MH_DYLIB_STUB> IsTypeDynamicLibraryStub{Filetype};
    BoundFlagEq<MH_DSYM> IsTypeDebugSymbols{Filetype};
    BoundFlagEq<MH_KEXT_BUNDLE> IsTypeKernelExtensionBundle{Filetype};

    // Flags
    BoundFlagAnd<MH_NOUNDEFS> HasNoUndefs{Raw.flags};
    BoundFlagAnd<MH_INCRLINK> IsIncrementallyLinked{Raw.flags};
    BoundFlagAnd<MH_DYLDLINK> IsDynamicallyLinked{Raw.flags};
    BoundFlagAnd<MH_BINDATLOAD> IsBoudndAtLoading{Raw.flags};
    BoundFlagAnd<MH_PREBOUND> IsPrebound{Raw.flags};
    BoundFlagAnd<MH_SPLIT_SEGS> HasSplitSegments{Raw.flags};
    BoundFlagAnd<MH_LAZY_INIT> IsLazilyInitialized{Raw.flags};
    BoundFlagAnd<MH_TWOLEVEL> IsTwoLevel{Raw.flags};
    BoundFlagAnd<MH_FORCE_FLAT> IsForcingFlat{Raw.flags};
    BoundFlagAnd<MH_NOMULTIDEFS> HasNoMultipleDefinitions{Raw.flags};
    BoundFlagAnd<MH_NOFIXPREBINDING> HasNoFixPrebinding{Raw.flags};
    BoundFlagAnd<MH_PREBINDABLE> IsBoundToAllModules{Raw.flags};
    BoundFlagAnd<MH_SUBSECTIONS_VIA_SYMBOLS> HasSymSubsections{Raw.flags};
    BoundFlagAnd<MH_CANONICAL> IsCanonial{Raw.flags};
    BoundFlagAnd<MH_WEAK_DEFINES> HasWeakSymbols{Raw.flags};
    BoundFlagAnd<MH_BINDS_TO_WEAK> UsesWeakSymbols{Raw.flags};
    BoundFlagAnd<MH_ALLOW_STACK_EXECUTION> AllowsStackExec{Raw.flags};
    BoundFlagAnd<MH_ROOT_SAFE> SafeForRoot{Raw.flags};
    BoundFlagAnd<MH_SETUID_SAFE> SafeForSetUID{Raw.flags};
    BoundFlagAnd<MH_NO_REEXPORTED_DYLIBS> HasNoReExpLibraries{Raw.flags};
    BoundFlagAnd<MH_PIE> IsPIE{Raw.flags};
    BoundFlagAnd<MH_DEAD_STRIPPABLE_DYLIB> IsDeadStripDyLib{Raw.flags};
    BoundFlagAnd<MH_HAS_TLV_DESCRIPTORS> HasTLVDescriptors{Raw.flags};
    BoundFlagAnd<MH_NO_HEAP_EXECUTION> HasNonExecutableHeap{Raw.flags};
    BoundFlagAnd<MH_APP_EXTENSION_SAFE> IsAppExtensionSafe{Raw.flags};

  public:
    bool Parse(std::istream &Input) {
      Filetype = Raw.filetype;
      return true;
    }
  };

  class MachOSection : public MachOThing<SectionCmd_t> {
  public:
    using MachOThing<SectionCmd_t>::Raw;
    std::string Name;
    std::string SegmentName;

    decltype(Raw.addr) VirtualAddress;
    decltype(Raw.size) VirtualSize;
    decltype(Raw.offset) FileOffset;

  public:
    void ApplyVirtualMemorySlide(uint64_t Value) { VirtualAddress += Value; }
    bool Parse(std::istream &Input) {
      Name = std::string(Raw.sectname);
      SegmentName = std::string(Raw.segname);
      return true;
    }
  };

  class MachOSegment : public MachOThing<SegmentCmd_t> {
  public:
    using MachOThing<SegmentCmd_t>::Raw;
    std::vector<std::shared_ptr<MachOSection>> Sections;
    std::string Name;

    decltype(Raw.vmaddr) VirtualAddress;
    decltype(Raw.vmsize) VirtualSize;
    decltype(Raw.fileoff) FileOffset;
    decltype(Raw.filesize) FileSize;

  public:
    void ApplyVirtualMemorySlide(uint64_t Value) {
      VirtualAddress += Value;
      for (auto &Section : Sections) {
        Section->ApplyVirtualMemorySlide(Value);
      }
    }
    bool Parse(std::istream &Input) {
      Name = std::string(Raw.segname);
      VirtualAddress = Raw.vmaddr;
      VirtualSize = Raw.vmsize;
      FileOffset = Raw.fileoff;
      FileSize = Raw.filesize;

      for (uint32_t s = 0; s < Raw.nsects; ++s) {
        ReadAThingFromInputAndPush(Input, Sections);
      }

      return true;
    }

    std::shared_ptr<MachOSection> GetSectionByName(std::string Name) {
      for (auto &Section : Sections) {
        if (Section.Name == Name) {
          return Section;
        }
      }
      return nullptr;
    }
  };

  class MachOSymbolTableEntry : public MachOThing<NList_t> {

  public:
    using MachOThing<NList_t>::Raw;
    std::string Name;

    // n_type::N_STAB
    uint8_t StubType;
    bool IsStub;

    // n_type::N_PEXT
    bool IsPrivateExternal;

    // n_type::N_TYPE
    bool IsUndefined;
    bool IsAbsolute;
    bool IsDefined;
    bool IsPrebound;
    bool IsIndirect;

    // n_type::N_EXT
    bool IsExtenal;

    // n_sect
    uint8_t SectionNumber;

    // n_desc
    uint32_t LineNumber;
    uint32_t NestingLevel;
    uint32_t Alignment;
    // REFERENCE_FLAG_* flags
    bool IsReferenceUndefinedNonLazy;
    bool IsReferenceUndefinedLazy;
    bool IsReferenceDefined;
    bool IsReferencePrivateDefined;
    bool IsReferencePrivateUndefinedNonLazy;
    bool IsReferencePrivateUndefinedLazy;
    // Additional flags
    bool IsReferenceDynamically;
    bool IsNoDeadStrip;
    bool IsDescDiscarded;
    bool IsWeakReference;
    bool IsWeakDefinition;
    bool IsReferenceToWeak;
    bool IsArmThumbDefinition;
    bool IsSymbolResolver;
    bool IsAltEntry;
    //
    uint32_t LibraryOrdinal;

    // n_value
    uint64_t Value;

  private:
    void ParseDesc(MachOParser &Parser) {
      // Common symbols have N_TYPE = U_UNDF | U_EXT
      if (IsUndefined && IsExtenal) {
        Alignment = GET_COMM_ALIGN(Raw.n_desc);
      }

      auto ReferenceType = Raw.n_desc & REFERENCE_TYPE;
      IsReferenceUndefinedNonLazy =
          ReferenceType == REFERENCE_FLAG_UNDEFINED_NON_LAZY;
      IsReferenceUndefinedLazy = ReferenceType == REFERENCE_FLAG_UNDEFINED_LAZY;
      IsReferenceDefined = ReferenceType == REFERENCE_FLAG_DEFINED;
      IsReferencePrivateDefined =
          ReferenceType == REFERENCE_FLAG_PRIVATE_DEFINED;
      IsReferencePrivateUndefinedNonLazy =
          ReferenceType == REFERENCE_FLAG_PRIVATE_UNDEFINED_NON_LAZY;
      IsReferencePrivateUndefinedLazy =
          ReferenceType == REFERENCE_FLAG_PRIVATE_UNDEFINED_LAZY;

      IsReferenceDynamically = Raw.n_desc & REFERENCED_DYNAMICALLY;

      if (Parser.Header->IsTypeObject) {
        IsNoDeadStrip = Raw.n_desc & N_NO_DEAD_STRIP;
        IsSymbolResolver = Raw.n_desc & N_SYMBOL_RESOLVER;
      }

      if (Parser.IsImage) {
        IsDescDiscarded = Raw.n_desc & N_DESC_DISCARDED;
      }

      IsWeakReference = Raw.n_desc & N_WEAK_REF;
      IsWeakDefinition = Raw.n_desc & N_WEAK_DEF;
      IsReferenceToWeak = Raw.n_desc & N_REF_TO_WEAK;
      IsArmThumbDefinition = Raw.n_desc & N_ARM_THUMB_DEF;
      IsAltEntry = Raw.n_desc & N_ALT_ENTRY;

      if (Parser.Header->IsTwoLevel) {
        LibraryOrdinal = GET_LIBRARY_ORDINAL(Raw.n_desc);
      }
    }

    void ParseStub(MachOParser &Parser) {
      StubType = Raw.n_type;

      switch (StubType) {
      case N_FUN:
      case N_SLINE:
      case N_ENTRY:
        LineNumber = Raw.n_desc;
        break;
      case N_LBRAC:
      case N_RBRAC:
        NestingLevel = Raw.n_desc;
        break;
      case N_GSYM:
      case N_STSYM:
      case N_LCSYM:
      case N_RSYM:
      case N_SSYM:
      case N_LSYM:
      case N_PSYM:
        ParseDesc(Parser);
        break;
      }
    }

  public:
    // TODO init all the fields
    MachOSymbolTableEntry()
        : StubType(0), SectionNumber(0), LineNumber(0), NestingLevel(0),
          Value(0) {}

    // Do processing and specifically string retrieval in post-parse call so we
    // would not jump memory between symbols and strings often
    bool PostParse(MachOParser &Parser) {
      auto &Input = Parser.Input;
      auto StringTableOffset = Parser.SymbolTable->StringTableOffset;
      auto StringTableSize = Parser.SymbolTable->StringTableSize;

      SectionNumber = Raw.n_sect;
      Value = Raw.n_value + Parser.ImageSlide;

      IsStub = Raw.n_type & N_STAB;
      IsPrivateExternal = Raw.n_type & N_PEXT;
      IsExtenal = Raw.n_type & N_EXT;

      auto NType = Raw.n_type & N_TYPE;
      IsUndefined = NType == N_UNDF;
      IsAbsolute = NType == N_ABS;
      IsDefined = NType == N_SECT;
      IsPrebound = NType == N_PBUD;
      IsIndirect = NType == N_INDR;

      if (IsStub) {
        ParseStub(Parser);
      } else {
        ParseDesc(Parser);
      }

      // For whatever reason symbol table may contain invalid records with
      // n_strx pointing way beyond its string table limits. Dynamic Loader
      // just skips those, so does this parser.
      auto Index = Raw.n_un.n_strx;
      if (Index > StringTableSize) {
        return true;
      }

      // Zero string table offsets means there is no name for the thing
      if (Index) {
        Input.seekg(StringTableOffset + Index);
        Name = ReadNTStringFromInput(Input);
      }

      return true;
    }
  };

  class MachOSymbolTable : public MachOThing<symtab_command> {
  public:
    using MachOThing<symtab_command>::Raw;
    std::vector<std::shared_ptr<MachOSymbolTableEntry>> Symbols;

    decltype(Raw.stroff) StringTableOffset;
    decltype(Raw.strsize) StringTableSize;

  public:
    bool PostParse(MachOParser &Parser) {
      // We know that symbol table records reside in __LINKEDIT segment, but
      // the symoff is given relative to the object file and not segment
      // itself. We have to subtract segment fileoff from this value to get
      // segment relative offset.
      if (auto LinkEdit = Parser.GetSegmentByName(SEG_LINKEDIT)) {
        auto &Input = Parser.Input;

        uint64_t LinkEditOffset =
            Parser.IsImage ? LinkEdit->VirtualAddress - Parser.ImageAddress
                           : LinkEdit->FileOffset;

        StringTableOffset = LinkEditOffset + Raw.stroff - LinkEdit->FileOffset;
        StringTableSize = Raw.strsize;

        Input.seekg(LinkEditOffset + Raw.symoff - LinkEdit->FileOffset);
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
  BoundFlagEq<MO_PARSE_IMAGE> IsImage{Flags};
  BoundFlagEq<MO_PARSE_FILE> IsFile{Flags};

  // VirtualAddress of the image we parse
  uint64_t ImageAddress;
  uint64_t ImageSlide;

public:
  std::shared_ptr<MachOHeader> Header;
  std::vector<std::shared_ptr<MachOSegment>> Segments;
  std::vector<std::shared_ptr<MachOSection>> Sections;
  std::shared_ptr<MachOSymbolTable> SymbolTable;
  std::shared_ptr<MachODySymbolTable> DySymbolTable;
  std::vector<std::shared_ptr<MachODyLibrary>> DyLibraries;
  std::shared_ptr<MachODyLibrary> DyLibraryId;
  std::shared_ptr<MachODyLinker> DyLinker;
  std::shared_ptr<MachODyLinker> DyLinkerId;

public:
  MachOParser(std::string Label, std::istream &Input, uint32_t Flags,
              uint64_t ImageAddress = 0)
      : Label(Label), Input(Input), Flags(Flags), ImageAddress(ImageAddress),
        ImageSlide(0) {}

  std::shared_ptr<MachOSegment> GetSegmentByName(std::string Name) {
    for (auto &Segment : Segments) {
      if (Segment->Name == Name) {
        return Segment;
      }
    }
    return nullptr;
  }

  std::shared_ptr<MachOSegment> GetSegmentByIndex(unsigned Index) {
    assert(Index && "Index must not be zero");
    if (Segments.size() >= Index) {
      return Segments.at(Index - 1);
    }
    return nullptr;
  }

  std::shared_ptr<MachOSection> GetSectionByIndex(unsigned Index) {
    assert(Index && "Index must not be zero");
    if (Sections.size() >= Index) {
      return Sections.at(Index - 1);
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
        auto Segment = Segments.back();
        break;
      }

      case LC_SYMTAB: {
        ReadAThingFromInput(Input, SymbolTable);
        assert(SymbolTable);
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
        Error Err(MAD_ERROR_PARSER);
        Err.Log("Faild to parse", Label, "at", mainptr);
        return false;
      }

      mainptr += loadcmd.cmdsize;
    }

    return PostParse();
  }

private:
  bool PostParse() {
    HandleASLR();

    if (SymbolTable) {
      SymbolTable->PostParse(*this);
    }

    // Push every segment's sections into Sections vector so they could be
    // retrieved via appearance index
    for (auto &Segment : Segments) {
      Sections.insert(Sections.end(), Segment->Sections.begin(),
                      Segment->Sections.end());
    }

    return true;
  }

  // FIXME: Design Issue:
  // This seems like a bad choice to handle ASLR in parser, but without knowing
  // the slide it is not possible(in general case) to parse symbol table in
  // memory.
  void HandleASLR() {
    // DyLD is a subject to ASLR(though it is a non-pie binary), and this works
    // because MacOS's x86-64 only user-space code model is very similar to
    // AMD64 Small PIE. In this case we have to adjust provided value to
    // pinpoint the symbol we are looking for.
    if (IsImage && (Header->IsPIE || Header->IsTypeDynamicLinker)) {

      // Slide is the distance between requested virtual address and assigned
      // after ASLR. It is calculated by subtracting vmaddr of the __TEXT
      // segment from the image virtual address. It is common vmaddr of the
      // __TEXT to be 0.
      //
      // NOTE:
      // DyLD, though, does not use __TEXT segment directly but rather
      // selects a segment that has no file offset (fileoff == 0) and has
      // non-zero size (filesize != 0) which selects __TEXT.
      ImageSlide = ImageAddress - GetSegmentByName(SEG_TEXT)->VirtualAddress;

      for (auto &Segment : Segments) {
        Segment->ApplyVirtualMemorySlide(ImageSlide);
      }
    }
  }
};

using MachOParser32 = MachOParser<MachSystem32_t>;
using MachOParser64 = MachOParser<MachSystem64_t>;

} // namespace mad

#endif /* end of include guard: MACHOPARSER_HPP_L6XJ5WJN */
