// Std
#include <sstream>

// MAD
#include "MAD/Prompt.hpp"

using namespace mad;

bool PromptCmd::Parse(std::deque<std::string> &Arguments) {
  try {
    Parser.ParseArgs(Arguments.begin(), Arguments.end());
  } catch (args::Help) {
    std::cout << Parser;
    return false;
  } catch (args::ParseError e) {
    std::cerr << e.what() << std::endl;
    std::cerr << Parser;
    return false;
  } catch (args::ValidationError e) {
    std::cerr << e.what() << std::endl;
    std::cerr << Parser;
    return false;
  }
  return true;
}

Prompt::Prompt(std::string Name) : Name(Name) {
  AddCommand(std::make_shared<PromptCmdMadHelp>());
  AddCommand(std::make_shared<PromptCmdMadExit>());
  AddCommand(std::make_shared<PromptCmdBreakpointSet>());
  AddCommand(std::make_shared<PromptCmdProcessRun>());
  AddCommand(std::make_shared<PromptCmdProcessContinue>());
}

static inline std::deque<std::string> Tokenize(std::string String,
                                               char Delimiter) {
  std::deque<std::string> Result;
  std::stringstream Input{String};
  std::string S;

  while (std::getline(Input, S, Delimiter)) {
    Result.push_back(S);
  }

  return Result;
}

std::shared_ptr<PromptCmd> Prompt::Show() {
  const char *Line = linenoise(Name.c_str());

  if (!Line) {
    Line = "e";
  }

  if (!strlen(Line)) {
    return nullptr;
  }

  auto Tokens = Tokenize(Line, ' ');
  auto First = Tokens.front();
  Tokens.pop_front();

  std::shared_ptr<PromptCmd> Cmd;

  if (ShortcutToCommand.count(First)) {
    Cmd = ShortcutToCommand.at(First);
  } else if (GroupToCommands.count(First) &&
             GroupToCommands[First].count(Tokens.front())) {
    Cmd = GroupToCommands[First][Tokens.front()];
    Tokens.pop_front();
  }

  if (Cmd && Cmd->Parse(Tokens)) {
    return Cmd;
  }

  return nullptr;
}

void Prompt::ShowCommands() {
  Say("");
  Say("Debugger commands:");
  for (auto &GroupPair : GroupToCommands) {
    for (auto &CmdPair : GroupPair.second) {
      printf("  %-20s -- %s\n", (GroupPair.first + " " + CmdPair.first).c_str(),
             CmdPair.second->Desc.c_str());
    }
  }
  Say("");
  Say("Commands shortcuts:");
  for (auto &ShortPair : ShortcutToCommand) {
    auto Group = PromptCmdGroupToString(ShortPair.second->Group).c_str();
    auto SName = ShortPair.second->Name.c_str();
    auto Short = ShortPair.second->Shortcut.c_str();
    printf("  %-10s -- %s %s\n", Short, Group, SName);
  }
}

void Prompt::ShowHelp() {
  for (auto &GroupPair : GroupToCommands) {
    Say("");
    Say("GROUP", GroupPair.first);
    for (auto &CmdPair : GroupPair.second) {
      Say("  COMMAND", CmdPair.first);
      CmdPair.second->Parser.Help(std::cout);
    }
  }
}
