#ifndef debugger_HPP_BUXYKXVV
#define debugger_HPP_BUXYKXVV

#include <map>
#include <memory>
#include <string>
#include <unistd.h>
#include <vector>

#include "MAD/Breakpoint.hpp"
#include "MAD/MachMemory.hpp"
#include "MAD/MachProcess.hpp"
#include "MAD/Prompt.hpp"

namespace mad {

class Debugger {
private:
  Prompt Prompt;
  MachProcess Process;
  MachTask &Task;
  MachMemory &Memory;
  std::map<uintptr_t, std::shared_ptr<Breakpoint>> BreakpointsByAddress;
  std::map<uintptr_t, std::vector<std::shared_ptr<Breakpoint>>>
      BreakpointsByPage;

private:
  void StartDebugging();
  std::shared_ptr<Breakpoint> CreateBreakpoint(vm_address_t Address);

public:
  Debugger(std::string exec)
      : Prompt("MAD "), Process(exec), Task(Process.GetTask()), Memory(Task.GetMemory()) {}
  int Start();
};

} // namespace mad

#endif /* end of include guard: debugger_HPP_BUXYKXVV */
