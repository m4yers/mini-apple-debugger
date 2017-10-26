#ifndef PROMPT_HPP_5CZETNAV
#define PROMPT_HPP_5CZETNAV

// Std
#include <deque>
#include <map>
#include <memory>
#include <string>
#include <vector>

// Prompt
#include "args/args.hxx"
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

enum class PromptCmdType { EXIT, HELP, RUN, BREAKPOINT_SET, CONTINUE };
static inline std::string PromptCmdTypeToString(PromptCmdType Type) {
  switch (Type) {
  case PromptCmdType::EXIT:
    return "exit";
  case PromptCmdType::HELP:
    return "help";
  case PromptCmdType::RUN:
    return "run";
  case PromptCmdType::CONTINUE:
    return "continue";
  case PromptCmdType::BREAKPOINT_SET:
    return "set";
  }
}

class Prompt;
class PromptCmd {
  friend Prompt;

protected:
  Error Err;
  args::ArgumentParser Parser;
  args::HelpFlag Help{Parser, "HELP", "Show this help menu.", {'h', "help"}};
  PromptCmdGroup Group;
  PromptCmdType Type;
  std::string Name;
  std::string Shortcut; // Top-level shortcut
  std::string Desc;

public:
  PromptCmd(PromptCmdGroup Group, PromptCmdType Type, std::string Name,
            std::string Shortcut = nullptr, std::string Desc = "")
      : Parser(Desc), Group(Group), Type(Type), Name(Name), Shortcut(Shortcut),
        Desc(Desc) {}
  bool Parse(std::deque<std::string> &Arguments);
  auto GetGroup() { return Group; }
  auto GetType() { return Type; }
  bool IsValid() { return Err.Success(); }
  void PrintHelp() {}
};

//-----------------------------------------------------------------------------
// MAD
//-----------------------------------------------------------------------------
class PromptCmdMadHelp : public PromptCmd {
public:
  PromptCmdMadHelp()
      : PromptCmd(PromptCmdGroup::MAD, PromptCmdType::CONTINUE, "help", "?") {}
};

class PromptCmdMadExit : public PromptCmd {
public:
  PromptCmdMadExit()
      : PromptCmd(PromptCmdGroup::MAD, PromptCmdType::EXIT, "exit", "e") {}
};

//-----------------------------------------------------------------------------
// Breakpoint
//-----------------------------------------------------------------------------
class PromptCmdBreakpointSet : public PromptCmd {
public:
  args::Group TargetGroup{Parser, "One of these must be specified",
                          args::Group::Validators::AtLeastOne};
  args::ValueFlag<std::string> SymbolName{
      TargetGroup, "SYMBOL", "Name of a symbol", {'n', "name"}};
  args::ValueFlag<std::string> MethodName{
      TargetGroup, "METHOD", "Name of a method", {'m', "method"}};

public:
  PromptCmdBreakpointSet()
      : PromptCmd(PromptCmdGroup::BREAKPOINT, PromptCmdType::BREAKPOINT_SET,
                  "set", "b") {}
};

//-----------------------------------------------------------------------------
// Process
//-----------------------------------------------------------------------------
class PromptCmdProcessRun : public PromptCmd {
public:
  PromptCmdProcessRun()
      : PromptCmd(PromptCmdGroup::PROCESS, PromptCmdType::RUN, "run", "r") {}
};

class PromptCmdProcessContinue : public PromptCmd {
public:
  PromptCmdProcessContinue()
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
    std::cout << std::forward<T>(p) << " ";
    sout.print(std::forward<Ts>(ps)...);
  }
  template <typename T> void Say(T &&p) {
    std::cout << std::forward<T>(p) << std::endl;
  }
};
} // namespace mad

#endif /* end of include guard: PROMPT_HPP_5CZETNAV */
