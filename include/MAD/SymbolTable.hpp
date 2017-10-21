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
  std::map<std::string, std::shared_ptr<SymbolEntry_t>> SymbolsByName;

public:
  SymbolTable(MachOParser<T> &Parser) : Parser(Parser) {}

  bool Init() {
    for (auto Entry : Parser.SymbolTable->Symbols) {
      SymbolsByName.insert({Entry->Name, Entry});
    }
    return true;
  }

  auto GetSymbols() {
    assert(Parser.Header);
    return Parser.SymbolTable->Symbols;
  }

  std::shared_ptr<SymbolEntry_t> GetSymbolByName(std::string Name) {
    if (SymbolsByName.count(Name)) {
      return SymbolsByName.at(Name);
    }
    return nullptr;
  }
};
} // namespace mad

#endif /* end of include guard: SYMBOLTABLE_HPP_D8BWWYFY */
