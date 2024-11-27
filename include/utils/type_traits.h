#ifndef XUBINH_SERVER_UTILS_TYPE_TRAITS
#define XUBINH_SERVER_UTILS_TYPE_TRAITS

#include <type_traits>

namespace xubinh_server {

namespace utils {

namespace type_traits {

// helper for SFINAE, which will be introduced later in C++14
template <bool C, typename T = void>
using enable_if_t = typename std::enable_if<C, T>::type;

} // namespace type_traits

} // namespace utils

} // namespace xubinh_server

#endif