#ifndef XUBINH_SERVER_UTILS_FORMAT
#define XUBINH_SERVER_UTILS_FORMAT

#include <array>
#include <cstddef>
#include <type_traits>

namespace xubinh_server {

namespace utils {

namespace Format {

// declaration
template <typename T>
int get_number_of_digits_of_type();
// has to be non-constexpr since explicit instantiation of constexpr is not
// allowed in C++11, and here I chose explicit instantiation over constexpr for
// limiting possible tempalte argments

// declaration
template <typename T>
size_t convert_integer_to_string_base_10(char *output_buffer, T input_value);

void convert_pointer_to_hex_string(char *output_buffer, const void *pointer);

} // namespace Format

} // namespace utils

} // namespace xubinh_server

#endif