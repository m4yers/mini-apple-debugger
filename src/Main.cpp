#include <iostream>

#include "MAD/Debugger.hpp"

#include "MAD/Debug.hpp"

using namespace mad;

int main(int argc, char *argv[]) {
  if (argc < 2) {
    PRINT_ERROR("Expected a program name as argument");
    return -1;
  }

  Debugger D(argv[1]);
  return D.Run();
}
