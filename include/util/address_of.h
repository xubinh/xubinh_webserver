#ifndef XUBINH_SERVER_UTIL_ADDRESS_OF
#define XUBINH_SERVER_UTIL_ADDRESS_OF

#include <type_traits>

#include "util/type_traits.h"

namespace xubinh_server {

namespace util {

template <
    typename T,
    typename = type_traits::enable_if_t<std::is_reference<T>::value>>
constexpr inline typename std::remove_reference<T>::type *address_of(T &&o
) noexcept {
    return __builtin_addressof(o);
}

} // namespace util

} // namespace xubinh_server

#endif