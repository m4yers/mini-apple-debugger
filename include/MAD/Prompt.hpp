#ifndef PROMPT_HPP_5CZETNAV
#define PROMPT_HPP_5CZETNAV

// Std
#include <string>

// Prompt
#include "linenoise/linenoise.h"

namespace mad {
class Prompt {
  std::string Name;

public:
  Prompt(std::string Name) : Name(Name) {}

  void Show() {
    auto Line = linenoise(Name.c_str());
  }
};
} // namespace mad

#endif /* end of include guard: PROMPT_HPP_5CZETNAV */
