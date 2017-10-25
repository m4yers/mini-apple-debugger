#include <iostream>

#include "MAD/Debugger.hpp"

#include "MAD/Error.hpp"

using namespace mad;

int main(int argc, char *argv[]) {

  Debugger D;
  return D.Start(argc, argv);
}
