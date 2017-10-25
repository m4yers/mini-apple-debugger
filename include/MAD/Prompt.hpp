#ifndef PROMPT_HPP_5CZETNAV
#define PROMPT_HPP_5CZETNAV

// Std
#include <deque>
#include <map>
#include <memory>
#include <string>
#include <vector>

// Prompt
#include "linenoise/linenoise.h"

// MAD
#include <MAD/Error.hpp>

namespace mad {

//------------------------------------------------------------------------------
// Commands
//------------------------------------------------------------------------------
enum class PromptCmdGroup { MAD, PROCESS, BREAKPOINT };
static inline std::string PromptCmdGroupToString(PromptCmdGroup Group) {
  switch (Group) {
  case PromptCmdGroup::MAD:
    return "mad";
  case PromptCmdGroup::PROCESS:
    return "process";
  case PromptCmdGroup::BREAKPOINT:
    return "breakpoint";
  }
}

enum class PromptCmdType { EXIT, HELP, RUN, CONTINUE };
static inline std::string PromptCmdTypeToString(PromptCmdType Type) {
  switch (Type) {
  case PromptCmdType::HELP:
    return "help";
  case PromptCmdType::RUN:
    return "run";
  case PromptCmdType::CONTINUE:
    return "continue";
  }
}

class Prompt;
class PromptCmd {
  friend Prompt;
  Error Err;
  PromptCmdGroup Group;
  PromptCmdType Type;
  std::string Name;
  std::string Shortcut; // Top-level shortcut
  std::string Desc;

public:
  PromptCmd(PromptCmdGroup Group, PromptCmdType Type, std::string Name,
            std::string Shortcut = nullptr, std::string Desc = "")
      : Group(Group), Type(Type), Name(Name), Shortcut(Shortcut), Desc(Desc) {}
  void Parse(std::deque<std::string> Tokens) {
    Err = Error(MAD_ERROR_PROMPT);
    Err.Log("The", PromptCmdGroupToString(Group), PromptCmdTypeToString(Type),
            "command does not take any arguments.");
  }
  auto GetGroup() { return Group; }
  auto GetType() { return Type; }
  bool IsValid() { return Err.Success(); }
  void PrintHelp() {}
};

class PromptCmdHelp : public PromptCmd {
public:
  PromptCmdHelp()
      : PromptCmd(PromptCmdGroup::MAD, PromptCmdType::CONTINUE, "help", "?") {}
};

class PromptCmdExit : public PromptCmd {
public:
  PromptCmdExit()
      : PromptCmd(PromptCmdGroup::MAD, PromptCmdType::EXIT, "exit",
                  "e") {}
};

class PromptCmdRun : public PromptCmd {
public:
  PromptCmdRun()
      : PromptCmd(PromptCmdGroup::PROCESS, PromptCmdType::RUN, "run",
                  "r") {}
};

class PromptCmdContinue : public PromptCmd {
public:
  PromptCmdContinue()
      : PromptCmd(PromptCmdGroup::PROCESS, PromptCmdType::CONTINUE, "continue",
                  "c") {}
};

//------------------------------------------------------------------------------
// Prompt
//------------------------------------------------------------------------------
class Prompt {
  std::string Name;
  std::map<std::string, std::shared_ptr<PromptCmd>> ShortcutToCommand;

private:
  void AddCommand(std::shared_ptr<PromptCmd> Cmd) {
    if (Cmd->Shortcut.size()) {
      ShortcutToCommand.emplace(Cmd->Shortcut, Cmd);
    }
  }

public:
  Prompt(std::string Name);
  std::shared_ptr<PromptCmd> Show();
  void ShowHelp();

  template <typename T, typename... Ts> void Say(T &&p, Ts &&... ps) {
    std::cout << std::forward<T>(p);
    sout.print(std::forward<Ts>(ps)...);
  }
  template <typename T> void Say(T &&p) {
    std::cout << std::forward<T>(p) << std::endl;
  }
};
} // namespace mad

#endif /* end of include guard: PROMPT_HPP_5CZETNAV */
