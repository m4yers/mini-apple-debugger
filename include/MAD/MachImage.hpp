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
      : Task(Task), Address(Address), MemoryStream(Task, Address),
        Parser(Name, MemoryStream, MO_PARSE_IMAGE), SymbolTable(Parser) {}

  bool Scan() { return Parser.Parse() && SymbolTable.Init(); }
  auto &GetSymbolTable() { return SymbolTable; }
};

using MachImage32 = MachImage<MachSystem32_t>;
using MachImage64 = MachImage<MachSystem64_t>;
} // namespace mad

#endif /* end of include guard: MACHIMAGE_HPP_12TQWXQI */
