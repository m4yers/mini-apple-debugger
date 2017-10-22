#ifndef MACHIMAGE_HPP_12TQWXQI
#define MACHIMAGE_HPP_12TQWXQI

#include <mach/mach.h>

#include <MAD/Debug.hpp>
#include <MAD/Mach.hpp>
#include <MAD/MachOParser.hpp>
#include <MAD/MachTask.hpp>
#include <MAD/MachTaskMemoryStream.hpp>
#include <MAD/SymbolTable.hpp>

namespace mad {
template <typename T, typename = IsMachSystem_t<T>> class MachImage {
  MachTask &Task;
  vm_address_t Address;
  MachTaskMemoryStream MemoryStream;
  MachOParser<T> Parser;
  SymbolTable<T> SymbolTable;

private:
  void HandlerASLR() {
    auto Header = Parser.Header;

    // DyLD is a subject to ASLR(though it is a non-pie binary), and this works
    // because MacOS's x86-64 only user-space code model is very similar to
    // AMD64 Small PIE. In this case we have to adjust provided value to
    // pinpoint the symbol we are looking for.
    if (Header->IsPIE || Header->IsTypeDynamicLinker) {
      for (auto &Symbol : Parser.SymbolTable->Symbols) {

        // Slide is the distance between requested virtual address and assigned
        // after ASLR. It is calculated by subtracting vmaddr of the __TEXT
        // segment from the image virtual address. It is common vmaddr of the
        // __TEXT to be 0.
        //
        // NOTE:
        // DyLD, though, does not use __TEXT segment directly but rather
        // selects a segment that has no file offset (fileoff == 0) and has
        // non-zero size (filesize != 0) which selects __TEXT.
        auto Slide = Address - GetTextSegment()->VirtualAddress;

        // Because symbol's address is virtual(not an offset from the start of
        // the segment or file) we add this slide to the value of the symbol.
        Symbol->Value += Slide;
      }
    }
  }

public:
  MachImage(std::string Name, MachTask &Task, vm_address_t Address)
      : Task(Task), Address(Address), MemoryStream(Task.GetMemory(), Address),
        Parser(Name, MemoryStream, MO_PARSE_IMAGE), SymbolTable(Parser) {}

  MachImage(const MachImage &Other) = delete;
  MachImage &operator=(const MachImage &Other) = delete;

  MachImage(MachImage &&Other) = default;
  MachImage &operator=(MachImage &&Other) = default;

  bool Scan() {
    if (!Parser.Parse()) {
      return false;
    }
    HandlerASLR();
    SymbolTable.Init();

    return true;
  }

  auto GetType() { return Parser.Header->Filetype; }
  auto GetAddress() { return Address; }
  auto &GetSymbolTable() { return SymbolTable; }

  auto GetSegmentByName(std::string Name) {
    return Parser.GetSegmentByName(Name);
  }
  auto GetSegmentByIndex(unsigned Index) {
    return Parser.GetSegmentByIndex(Index);
  }
  auto GetSectionByIndex(unsigned Index) {
    return Parser.GetSectionByIndex(Index);
  }
  auto GetTextSegment() { return Parser.GetSegmentByName(SEG_TEXT); }
  auto GetDataSegment() { return Parser.GetSegmentByName(SEG_DATA); }
  auto GetLinkEditSegment() { return Parser.GetSegmentByName(SEG_LINKEDIT); }
};

using MachImage32 = MachImage<MachSystem32_t>;
using MachImage64 = MachImage<MachSystem64_t>;
} // namespace mad

#endif /* end of include guard: MACHIMAGE_HPP_12TQWXQI */
