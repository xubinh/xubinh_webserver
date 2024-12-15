#ifndef __XUBINH_SERVER_UTIL_TYPE_TRAITS
#define __XUBINH_SERVER_UTIL_TYPE_TRAITS

#include <type_traits>

namespace xubinh_server {

namespace util {

namespace type_traits {

template <bool C, typename T = void>
using enable_if_t = typename std::enable_if<C, T>::type;

template <bool C, typename T = void>
using disable_if_t = typename std::enable_if<!C, T>::type;

template <typename T>
using underlying_type_t = typename std::underlying_type<T>::type;

template <typename T>
using decay_t = typename std::decay<T>::type;

template <typename T>
using remove_reference_t = typename std::remove_reference<T>::type;

template <typename T>
using remove_cv_t = typename std::remove_cv<T>::type;

template <typename T>
using remove_pointer_t = typename std::remove_pointer<T>::type;

template <typename T>
struct is_rvalue_reference_or_normal_value {
    // true if T is a rvalue reference type or a normal non-reference type
    static constexpr bool value = std::is_rvalue_reference<T &&>::value;
};

template <typename T>
struct is_non_const_lvalue_reference {
    static constexpr bool value =
        (!is_rvalue_reference_or_normal_value<T>::value)
        && (!std::is_const<remove_reference_t<T>>::value);
};

} // namespace type_traits

} // namespace util

} // namespace xubinh_server

#endif