#ifndef debugger_HPP_BUXYKXVV
#define debugger_HPP_BUXYKXVV

#include <map>
#include <string>
#include <unistd.h>

#include "MAD/Breakpoint.hpp"
#include "MAD/MachProcess.hpp"

namespace mad {

class Debugger {
private:
  MachProcess Process;
  std::map<uintptr_t, Breakpoint> Breakpoints;

private:
  void StartDebugging();

public:
  Debugger(std::string exec) : Process(exec) {}
  int Run();
};

} // namespace mad

#endif /* end of include guard: debugger_HPP_BUXYKXVV */
