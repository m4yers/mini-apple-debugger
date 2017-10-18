#ifndef UNIVERSALBINARY_HPP_PMFJI0Q2
#define UNIVERSALBINARY_HPP_PMFJI0Q2

#include <fstream>
#include <map>
#include <string>

#include "MAD/ObjectFile.hpp"

namespace mad {
class UniversalBinary {
  std::string Path;
  std::ifstream &Input;
  std::map<uint32_t, ObjectFile> ObjectFiles;

public:
  UniversalBinary(std::string path, std::ifstream &input)
      : Path(path), Input(input) {}

  bool Parse();
};
} // namespace mad

#endif /* end of include guard: UNIVERSALBINARY_HPP_PMFJI0Q2 */
