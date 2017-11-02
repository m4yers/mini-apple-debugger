#ifndef debugger_HPP_BUXYKXVV
#define debugger_HPP_BUXYKXVV

// Std
#include <map>
#include <memory>
#include <string>
#include <unistd.h>
#include <vector>

// MAD
#include "MAD/BreakpointsControl.hpp"
#include "MAD/MachMemory.hpp"
#include "MAD/MachProcess.hpp"
#include "MAD/Prompt.hpp"

namespace mad {

class Debugger {
private:
  Prompt Prompt;
  std::string Exe;
  std::shared_ptr<MachProcess> Process;
  BreakpointsControl BreakpointsCtrl;

private:
  void HandleProcessContinue();
  void HandleProcessRun();
  void HandleProcessStop();

  void HandleBreakpointSet(const std::shared_ptr<PromptCmdBreakpointSet> &BPS);
  BreakpointCallbackReturn HandleSymbolNameBreakpoint(std::string);
  BreakpointBySymbolNameCallback_t HandleSymbolNameBreakpoint_l =
      [this](const auto &a) { return HandleSymbolNameBreakpoint(a); };

public:
  Debugger() : Prompt("(mad) "), Process(nullptr) {}
  int Start(int argc, char *argv[]);
};

} // namespace mad

#endif /* end of include guard: debugger_HPP_BUXYKXVV */
