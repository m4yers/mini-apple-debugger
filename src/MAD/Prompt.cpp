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
  }

  if (Cmd && Cmd->Parse(Tokens)) {
    PRINT_DEBUG("PROMT CHOSES", Cmd->Name);
    return Cmd;
  }

  return nullptr;
}
void Prompt::ShowHelp() {
  // TODO: Help here
}
