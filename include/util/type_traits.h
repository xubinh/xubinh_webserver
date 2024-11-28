#ifndef XUBINH_SERVER_UTIL_TYPE_TRAITS
#define XUBINH_SERVER_UTIL_TYPE_TRAITS

#include <type_traits>

namespace xubinh_server {

namespace util {

namespace type_traits {

// helper for SFINAE, which will be introduced later in C++14
template <bool C, typename T = void>
using enable_if_t = typename std::enable_if<C, T>::type;

template <typename T1, typename T2>
struct is_same_type {
    static constexpr bool value = false;
};

template <typename T>
struct is_same_type<T, T> {
    static constexpr bool value = true;
};

} // namespace type_traits

} // namespace util

} // namespace xubinh_server

#endif