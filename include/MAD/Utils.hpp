#ifndef UTILS_HPP_J9PMINOK
#define UTILS_HPP_J9PMINOK

#include <ostream>

namespace mad {

//-----------------------------------------------------------------------------
// Flag Policies
//-----------------------------------------------------------------------------
template <typename T> class FlagPolicyEq {
public:
  static bool IsTrue(T Base, T Value) { return Base == Value; }
};

template <typename T> class FlagPolicyAnd {
public:
  static bool IsTrue(T Base, T Value) { return Base & Value; }
};

//-----------------------------------------------------------------------------
// Flag
//-----------------------------------------------------------------------------
template <typename T, template <typename> class Policy, T Base> class Flag {
  bool Value;

public:
  Flag() : Value(false) {}
  explicit operator bool() const { return Value; }

  Flag &operator=(bool Value) {
    this->Value = Value;
    return *this;
  }
  Flag &operator=(T Value) {
    this->Value = Policy<T>::IsTrue(Base, Value);
    return *this;
  }
};

template <typename T, template <typename> class Policy, T Base>
std::ostream &operator<<(std::ostream &os, const Flag<T, Policy, Base> &Flag) {
  os << (bool)Flag;
  return os;
}

template <unsigned Value> using FlagEq = Flag<unsigned, FlagPolicyEq, Value>;
template <unsigned Value> using FlagAnd = Flag<unsigned, FlagPolicyAnd, Value>;

//-----------------------------------------------------------------------------
// BoundFlag
//-----------------------------------------------------------------------------
template <typename T, template <typename> class Policy, T Base>
class BoundFlag {
  T &Field;
  bool Value;

public:
  BoundFlag(T &Field) : Field(Field) {}
  explicit operator bool() const { return Policy<T>::IsTrue(Base, Field); }
};

template <typename T, template <typename> class Policy, T Base>
std::ostream &operator<<(std::ostream &os,
                         const BoundFlag<T, Policy, Base> &Flag) {
  os << (bool)Flag;
  return os;
}

template <unsigned Value>
using BoundFlagEq = BoundFlag<unsigned, FlagPolicyEq, Value>;
template <unsigned Value>
using BoundFlagAnd = BoundFlag<unsigned, FlagPolicyAnd, Value>;

} // namespace mad

#endif /* end of include guard: UTILS_HPP_J9PMINOK */
