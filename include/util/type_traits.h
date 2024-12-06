#ifndef XUBINH_SERVER_UTIL_TYPE_TRAITS
#define XUBINH_SERVER_UTIL_TYPE_TRAITS

#include <type_traits>

namespace xubinh_server {

namespace util {

namespace type_traits {

// introduced in C++14
template <bool C, typename T = void>
using enable_if_t = typename std::enable_if<C, T>::type;

// introduced in C++14
template <typename T>
using underlying_type_t = typename std::underlying_type<T>::type;

// introduced in C++14
template <typename T>
using decay_t = typename std::decay<T>::type;

} // namespace type_traits

} // namespace util

} // namespace xubinh_server

#endif