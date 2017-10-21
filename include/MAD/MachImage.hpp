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

public:
  MachImage(std::string Name, MachTask &Task, vm_address_t Address)
      : Task(Task), Address(Address), MemoryStream(Task.GetMemory(), Address),
        Parser(Name, MemoryStream, MO_PARSE_IMAGE), SymbolTable(Parser) {}

  MachImage(const MachImage &Other) = delete;
  MachImage &operator=(const MachImage &Other) = delete;

  MachImage(MachImage &&Other) = default;
  MachImage &operator=(MachImage &&Other) = default;

  bool Scan() { return Parser.Parse() && SymbolTable.Init(); }

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
