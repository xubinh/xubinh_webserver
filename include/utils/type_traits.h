#ifndef XUBINH_SERVER_UTILS_TYPE_TRAITS
#define XUBINH_SERVER_UTILS_TYPE_TRAITS

#include <type_traits>

namespace xubinh_server {

namespace utils {

namespace type_traits {

// introduced later in C++14
template <bool C, typename T>
using enable_if_t = typename std::enable_if<C, T>::type;

// SFINAE
template <typename T>
struct is_one_of_8_integer_types {};

template <>
struct is_one_of_8_integer_types<short> {
    bool value = true;
};
template <>
struct is_one_of_8_integer_types<int> {
    bool value = true;
};
template <>
struct is_one_of_8_integer_types<long> {
    bool value = true;
};
template <>
struct is_one_of_8_integer_types<long long> {
    bool value = true;
};
template <>
struct is_one_of_8_integer_types<unsigned short> {
    bool value = true;
};
template <>
struct is_one_of_8_integer_types<unsigned int> {
    bool value = true;
};
template <>
struct is_one_of_8_integer_types<unsigned long> {
    bool value = true;
};
template <>
struct is_one_of_8_integer_types<unsigned long long> {
    bool value = true;
};

// little helper
template <typename T>
using enable_if_is_one_of_8_integer_types_t =
    enable_if_t<is_one_of_8_integer_types<T>::value, T>;

} // namespace type_traits

} // namespace utils

} // namespace xubinh_server

#endif