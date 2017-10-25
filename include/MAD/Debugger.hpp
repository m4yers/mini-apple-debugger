#ifndef debugger_HPP_BUXYKXVV
#define debugger_HPP_BUXYKXVV

#include <map>
#include <memory>
#include <string>
#include <unistd.h>
#include <vector>

#include "MAD/Breakpoints.hpp"
#include "MAD/MachMemory.hpp"
#include "MAD/MachProcess.hpp"
#include "MAD/Prompt.hpp"

namespace mad {

class Debugger {
private:
  Prompt Prompt;
  std::string Exe;
  std::unique_ptr<MachProcess> Process;
  std::unique_ptr<Breakpoints> BreakpointsCtrl;

private:
  void StartDebugging();
  void WaitForDyLdToComplete();
  void HandleContinue();
  void HandleRun();

public:
  Debugger() : Prompt("(mad) "), Process(nullptr) {}
  int Start(int argc, char *argv[]);
};

} // namespace mad

#endif /* end of include guard: debugger_HPP_BUXYKXVV */
