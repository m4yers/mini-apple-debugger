#ifndef SYMBOLTABLE_HPP_D8BWWYFY
#define SYMBOLTABLE_HPP_D8BWWYFY

#include <map>
#include <memory>

#include <MAD/Debug.hpp>
#include <MAD/Mach.hpp>
#include <MAD/MachOParser.hpp>

namespace mad {
template <typename T, typename = IsMachSystem_t<T>> class SymbolTable {
  using SymbolEntry_t = typename MachOParser<T>::MachOSymbolTableEntry;

private:
  MachOParser<T> &Parser;
  std::map<std::string, std::shared_ptr<SymbolEntry_t>> NameToSymbol;

public:
  SymbolTable(MachOParser<T> &Parser) : Parser(Parser) {}

  bool Init() {
    for (auto Entry : Parser.SymbolTable->Symbols) {
      NameToSymbol.insert({Entry->Name, Entry});
    }
    return true;
  }

  auto GetSymbols() { return Parser.SymbolTable->Symbols; }

  auto GetSymbolByName(std::string Name) {
    if (NameToSymbol.count(Name)) {
      return NameToSymbol.at(Name);
    }
    return nullptr;
  }
};
} // namespace mad

#endif /* end of include guard: SYMBOLTABLE_HPP_D8BWWYFY */
