#ifndef DEBUG_HPP_2BQNSAUZ
#define DEBUG_HPP_2BQNSAUZ

#include <iostream>
#include <mach/kern_return.h>

class OutPrinter {
public:
  template <typename T, typename... Ts> void print(T &&p, Ts &&... ps) {
    std::cout << std::forward<T>(p);
    print(std::forward<Ts>(ps)...);
  }
  template <typename T> void print(T &&p) {
    std::cout << std::forward<T>(p) << std::endl;
  }
  template <typename T, typename... Ts> void error(T &&p, Ts &&... ps) {
    std::cerr << std::forward<T>(p);
    print(std::forward<Ts>(ps)...);
  }
  template <typename T> void error(T &&p) {
    std::cerr << std::forward<T>(p) << std::endl;
  }
  void endl() { std::cout << std::endl; }
};

extern OutPrinter sout;

extern std::string kern_return_to_string(kern_return_t kern_return);

#ifndef NDEBUG
#define NDEBUG 0
#endif

#if NDEBUG == 0
#define DEBUG(x)                                                               \
  { x; }
#define PRINT_DEBUG(...)                                                       \
  { sout.print(__VA_ARGS__); }
#else
#define DEBUG(x) 0
#define PRINT_DEBUG(...) 0
#endif

#define HEX(x) (void *)(intptr_t) x
#define PRINT_ERROR(...)                                                       \
  { sout.error(__FILE__, "$", __LINE__, ": ", __VA_ARGS__); }
#define PRINT_KERN_ERROR(x)                                                    \
  {                                                                            \
    sout.error(__FILE__, "$", __LINE__, ": KERN(", x, "), ",                   \
               std::strerror(errno));                                          \
  }
#define PRINT_KERN_ERROR_V(x, ...)                                             \
  {                                                                            \
    sout.error(__FILE__, "$", __LINE__, ": KERN(", x, "), ",                   \
               std::strerror(errno), __VA_ARGS__);                             \
  }

#endif /* end of include guard: DEBUG_HPP_2BQNSAUZ */
