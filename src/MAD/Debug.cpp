#include "MAD/Debug.hpp"

OutPrinter sout;

std::string kern_return_to_string(kern_return_t kern_return) {
  return "(" + std::to_string(kern_return) + ")";
}
