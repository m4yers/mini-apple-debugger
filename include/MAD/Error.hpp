#ifndef ERROR_HPP_FCRLWDDW
#define ERROR_HPP_FCRLWDDW

// System
#include <mach/mach.h>
#include <unistd.h>

// Std
#include <cerrno>
#include <iostream>
#include <string>

// MAD
#include <MAD/Debug.hpp>

namespace mad {

#define MAD_ERROR_BREAKPOINT 1
#define MAD_ERROR_PARSER 2
#define MAD_ERROR_MEMORY 3

using ErrorType = uint32_t;
using ErrorFlavour = enum {
  MAD = 0,
  Mach = 1,
  POSIX = 2,
};

inline std::string ErrorFlavourToString(ErrorFlavour Flavour) {
  switch (Flavour) {
  case MAD:
    return "MAD";
  case Mach:
    return "Mach";
  case POSIX:
    return "POSIX";
  }
}

class Error {
  ErrorType Value;
  ErrorFlavour Flavour;
  std::string Text;

public:
  static Error FromErrno() { return Error(errno, ErrorFlavour::POSIX); }

private:
  std::string GetErrorText(ErrorType Value, ErrorFlavour Flavour) {
    switch (Flavour) {
    case Mach:
      return mach_error_string(Value);
    case POSIX:
      return std::strerror(Value);
    default:
      return "";
    }
  }

public:
  Error(ErrorType Value = 0, ErrorFlavour Flavour = ErrorFlavour::MAD)
      : Value(Value), Flavour(Flavour), Text(GetErrorText(Value, Flavour)) {}

  Error &operator=(kern_return_t Kern) {
    Value = Kern;
    Flavour = ErrorFlavour::Mach;
    Text = GetErrorText(Value, Flavour);
    return *this;
  }

  explicit operator ErrorType() const { return Value; }
  explicit operator bool() const { return Fail(); }

  bool Success() const { return Value == 0; }
  bool Fail() const { return Value != 0; }

  std::string ToString() {
    auto Result = "(" + ErrorFlavourToString(Flavour) + ") ERROR " +
                  std::to_string(Value);

    if (Text.size()) {
      Result += ": " + Text;
    }

    return Result;
  }

  void Log() { sout.print(ToString()); }
  template <typename T, typename... Ts> void Log(T &&p, Ts &&... ps) {
    sout.print(ToString(), "\n  ", std::forward<T>(p), std::forward<Ts>(ps)...);
  }
  template <typename T> void Log(T &&p) {
    sout.print(ToString(), std::forward<T>(p));
  }
};
} // namespace mad

#endif /* end of include guard: ERROR_HPP_FCRLWDDW */
