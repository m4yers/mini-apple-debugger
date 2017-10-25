// Std
#include <sstream>

// MAD
#include "MAD/Prompt.hpp"

using namespace mad;

Prompt::Prompt(std::string Name) : Name(Name) {
  AddCommand(std::make_shared<PromptCmdHelp>());
  AddCommand(std::make_shared<PromptCmdExit>());
  AddCommand(std::make_shared<PromptCmdRun>());
  AddCommand(std::make_shared<PromptCmdContinue>());
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
  }

  if (Cmd) {
    PRINT_DEBUG("PROMT CHOSES", Cmd->Name);
    return Cmd;
  }

  return nullptr;
}
void Prompt::ShowHelp() {
  // TODO: Help here
}
