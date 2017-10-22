#ifndef UTILS_HPP_J9PMINOK
#define UTILS_HPP_J9PMINOK

namespace mad {
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

template <typename T> class FlagPolicy_Eq {
public:
  static bool IsTrue(T Base, T Value) { return Base == Value; }
};

template <typename T> class FlagPolicy_And {
public:
  static bool IsTrue(T Base, T Value) { return Base & Value; }
};

template <unsigned Value> using FlagEq = Flag<unsigned, FlagPolicy_Eq, Value>;
template <unsigned Value> using FlagAnd = Flag<unsigned, FlagPolicy_And, Value>;
} // namespace mad

#endif /* end of include guard: UTILS_HPP_J9PMINOK */
