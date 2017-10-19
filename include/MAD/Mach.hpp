
#ifndef MACH_HPP_AWUXJ6A0
#define MACH_HPP_AWUXJ6A0

#include <type_traits>

namespace mad {

using MachSystem32_t = std::integral_constant<int, 1>;
using MachSystem64_t = std::integral_constant<int, 0>;

template <typename T>
using IsMachSystem_t = std::enable_if_t<std::is_same_v<T, MachSystem32_t> ||
                                      std::is_same_v<T, MachSystem64_t>>;

} // namespace mad

#endif /* end of include guard: MACH_HPP_AWUXJ6A0 */
