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
  bool Result;

public:
  Flag() : Result(false) {}
  Flag(T Value) : Result(Policy<T>::IsTrue(Base, Value)) {}
  explicit operator bool() const { return Result; }

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

public:
  BoundFlag() = delete;
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

//-----------------------------------------------------------------------------
// Enum bitmasks
//
// NOTE: This can be used ONLY if enum covers ALL possbile bitmask values
//-----------------------------------------------------------------------------

#define ENUM_BITMASK_DEFINE_AND(E)                                             \
  inline E operator&(E x, E y) {                                               \
    return static_cast<E>(static_cast<std::underlying_type<E>::type>(x) &      \
                          static_cast<std::underlying_type<E>::type>(y));      \
  }
#define ENUM_BITMASK_DEFINE_OR(E)                                              \
  inline E operator|(E x, E y) {                                               \
    return static_cast<E>(static_cast<std::underlying_type<E>::type>(x) |      \
                          static_cast<std::underlying_type<E>::type>(y));      \
  }
#define ENUM_BITMASK_DEFINE_XOR(E)                                             \
  inline E operator^(E x, E y) {                                               \
    return static_cast<E>(static_cast<std::underlying_type<E>::type>(x) ^      \
                          static_cast<std::underlying_type<E>::type>(y));      \
  }

#define ENUM_BITMASK_DEFINE_AS_AND(E)                                          \
  inline E &operator&=(E &x, E y) {                                            \
    x = x & y;                                                                 \
    return x;                                                                  \
  }

#define ENUM_BITMASK_DEFINE_AS_OR(E)                                           \
  inline E &operator|=(E &x, E y) {                                            \
    x = x | y;                                                                 \
    return x;                                                                  \
  }

#define ENUM_BITMASK_DEFINE_AS_XOR(E)                                          \
  inline E &operator^=(E &x, E y) {                                            \
    x = x ^ y;                                                                 \
    return x;                                                                  \
  }

#define ENUM_BITMASK_DEFINE_ALL(E)                                             \
  ENUM_BITMASK_DEFINE_AND(E)                                                   \
  ENUM_BITMASK_DEFINE_XOR(E)                                                   \
  ENUM_BITMASK_DEFINE_OR(E)                                                    \
  ENUM_BITMASK_DEFINE_AS_AND(E)                                                \
  ENUM_BITMASK_DEFINE_AS_XOR(E)                                                \
  ENUM_BITMASK_DEFINE_AS_OR(E)

#endif /* end of include guard: UTILS_HPP_J9PMINOK */
