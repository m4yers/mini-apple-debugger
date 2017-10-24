#include <iostream>

#include "MAD/Debugger.hpp"

#include "MAD/Error.hpp"

using namespace mad;

int main(int argc, char *argv[]) {
  if (argc < 2) {
    Error Err(MAD_ERROR_ARGUMENTS);
    Err.Log("Expected a programm name as argument");
    return -1;
  }

  Debugger D(argv[1]);
  return D.Start();
}
