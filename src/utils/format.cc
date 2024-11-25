#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

#include "utils/format.h"

namespace xubinh_server {

namespace utils {

namespace Format {

namespace {

template <typename T>
struct _NumericLimits {
    static_assert(std::is_integral<T>::value, "T must be an integral type");

    static constexpr T max() {
        return std::is_signed<T>::value
                   ? static_cast<T>((T(1) << (sizeof(T) * 8 - 1)) - 1)
                   : static_cast<T>(~T(0));
    }

    static constexpr T min() {
        return std::is_signed<T>::value ? static_cast<T>(-max() - 1)
                                        : static_cast<T>(0);
    }
};

template <typename T>
constexpr int _compile_time_log10_floor(T integer_value) {
    static_assert(std::is_integral<T>::value, "T must be an integral type");

    return (integer_value < 10)
               ? 0
               : 1 + _compile_time_log10_floor(integer_value / 10);
}

} // namespace

// definition
template <typename T>
int get_number_of_digits_of_type() {
    static_assert(std::is_integral<T>::value, "T must be an integral type");

    return _compile_time_log10_floor(_NumericLimits<T>::max()) + 1;
}

// explicit instantiation
template int get_number_of_digits_of_type<short>();
template int get_number_of_digits_of_type<unsigned short>();
template int get_number_of_digits_of_type<int>();
template int get_number_of_digits_of_type<unsigned int>();
template int get_number_of_digits_of_type<long>();
template int get_number_of_digits_of_type<unsigned long>();
template int get_number_of_digits_of_type<long long>();
template int get_number_of_digits_of_type<unsigned long long>();

// definition
template <typename T>
size_t convert_integer_to_string_base_10(char *output_buffer, T input_value) {
    bool is_negative = std::is_signed<T>::value && (input_value < 0);

    char *buffer_ptr = output_buffer;

    do {
        *buffer_ptr++ = '0' + (input_value % 10);
        input_value /= 10;
    } while (input_value > 0);

    if (is_negative) {
        *buffer_ptr++ = '-';
    }

    *buffer_ptr = '\0';

    std::reverse(output_buffer, buffer_ptr);

    return buffer_ptr - output_buffer;
}

// explicit instantiation
template size_t convert_integer_to_string_base_10<short>(
    char *output_buffer, short input_value
);
template size_t convert_integer_to_string_base_10<unsigned short>(
    char *output_buffer, unsigned short input_value
);
template size_t
convert_integer_to_string_base_10<int>(char *output_buffer, int input_value);
template size_t convert_integer_to_string_base_10<unsigned int>(
    char *output_buffer, unsigned int input_value
);
template size_t
convert_integer_to_string_base_10<long>(char *output_buffer, long input_value);
template size_t convert_integer_to_string_base_10<unsigned long>(
    char *output_buffer, unsigned long input_value
);
template size_t convert_integer_to_string_base_10<long long>(
    char *output_buffer, long long input_value
);
template size_t convert_integer_to_string_base_10<unsigned long long>(
    char *output_buffer, unsigned long long input_value
);

namespace {

const char _hex_table[] = "0123456789abcdef";
constexpr size_t _NUMBER_OF_LETTERS_OF_POINTER_IN_HEX = sizeof(uintptr_t) * 2;

} // namespace

void convert_pointer_to_hex_string(char *output_buffer, const void *pointer) {
    uintptr_t value = reinterpret_cast<uintptr_t>(pointer);

    output_buffer[0] = '0';
    output_buffer[1] = 'x';

    auto buffer_ptr = output_buffer + 2;

    for (size_t i = 0; i < _NUMBER_OF_LETTERS_OF_POINTER_IN_HEX; ++i) {
        uint8_t nibble = value & 0xF;

        *buffer_ptr++ = _hex_table[nibble];

        value >>= (i * 4);
    }

    *buffer_ptr = '\0';

    std::reverse(output_buffer + 2, buffer_ptr);
}

} // namespace Format

} // namespace utils

} // namespace xubinh_server
