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

#define MAD_ERROR_ARGUMENTS 1u
#define MAD_ERROR_BREAKPOINT 2u
#define MAD_ERROR_PARSER 3u
#define MAD_ERROR_MEMORY 4u

using ErrorType = uint32_t;
using ErrorFlavour = enum {
  MAD,
  Mach,
  POSIX,
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

inline std::string MadErrorToString(unsigned Value) {
  switch (Value) {
  case MAD_ERROR_ARGUMENTS:
    return "ARGUMENTS";
  case MAD_ERROR_BREAKPOINT:
    return "BREAKPOINT";
  case MAD_ERROR_PARSER:
    return "PARSER";
  case MAD_ERROR_MEMORY:
    return "MEMORY";
  }
  return "UNKNOWN";
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
    case MAD:
      return MadErrorToString(Value);
    case Mach:
      return mach_error_string(Value);
    case POSIX:
      return std::strerror(Value);
    }
  }

public:
  Error(ErrorType Value = 0, ErrorFlavour Flavour = ErrorFlavour::MAD)
      : Value(Value), Flavour(Flavour), Text(GetErrorText(Value, Flavour)) {}
  Error(kern_return_t Value, ErrorFlavour Flavour = ErrorFlavour::Mach)
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
