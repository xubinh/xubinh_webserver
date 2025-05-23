#ifndef __XUBINH_SERVER_UTIL_FORMAT
#define __XUBINH_SERVER_UTIL_FORMAT

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <type_traits>

#include "util/type_traits.h"

namespace xubinh_server {

namespace util {

class Format {
public:
    // type dispatcher
    template <typename T>
    struct is_one_of_8_integer_types {
        static constexpr bool value = false;
    };

    // SFINAE
    template <typename T>
    using enable_if_is_one_of_8_integer_types_t =
        type_traits::enable_if_t<is_one_of_8_integer_types<T>::value>;

    // alias
    template <typename T>
    using enable_for_integer_types = enable_if_is_one_of_8_integer_types_t<T>;

    template <typename T, typename = enable_for_integer_types<T>>
    static constexpr int get_max_number_of_decimal_digits_of_integer_type() {
        return _log10_floor(std::numeric_limits<T>::max()) + 1
               + 1; // for possible leading minus sign `-`
    }

    template <typename T, typename Enable = void>
    struct get_min_number_of_chars_required_to_represent_value_of_type {};

    template <typename T>
    struct get_min_number_of_chars_required_to_represent_value_of_type<
        T,
        enable_for_integer_types<T>> {
        static constexpr size_t value =
            get_max_number_of_decimal_digits_of_integer_type<T>();
    };

    template <typename T>
    struct get_min_number_of_chars_required_to_represent_value_of_type<
        T *,
        void> {
        static constexpr size_t value = sizeof(T *) * 2 + 2; // for `0x` prefix
    };

    template <typename T, typename = enable_for_integer_types<T>>
    static size_t
    convert_integer_to_decimal_string(char *output_buffer, T input_value) {
        bool is_negative = std::is_signed<T>::value && (input_value < 0);

        // ignoring the negative-most case, e.g. for (signed) char, the two's
        // complement of -128 turns out to be exactly itself
        input_value = is_negative ? -input_value : input_value;

        char *buffer_ptr = output_buffer;

        do {
            *buffer_ptr++ = static_cast<char>('0' + (input_value % 10));
            input_value /= 10;
        } while (input_value > 0);

        if (is_negative) {
            *buffer_ptr++ = '-';
        }

        *buffer_ptr = '\0';

        std::reverse(output_buffer, buffer_ptr);

        return buffer_ptr - output_buffer;
    }

    static void
    convert_pointer_to_hex_string(char *output_buffer, const void *pointer) {
        output_buffer[0] = '0';
        output_buffer[1] = 'x';

        uintptr_t value = reinterpret_cast<uintptr_t>(pointer);

        auto buffer_ptr = output_buffer + 2;

        for (size_t i = 0; i < _NUMBER_OF_LETTERS_OF_POINTER_IN_HEX; i++) {
            uint8_t nibble = value & 0xF;

            *buffer_ptr++ = _HEX_ALPHABET_TABLE[nibble];

            value >>= 4;
        }

        *buffer_ptr = '\0';

        std::reverse(output_buffer + 2, buffer_ptr);
    }

    static constexpr const char *
    get_base_name_of_path(const char *path, size_t length) {
        return _get_base_name_of_path(path, length);
    }

    // [NOTE]: the argument must be a reference type in order to disable the
    // implicit `const char []` -> `const char *` conversion
    template <size_t N>
    static constexpr const char *get_base_name_of_path(const char (&path)[N]) {
        return _get_base_name_of_path(path, N - 1); // minus 1 for the `\0`
    }

    static constexpr size_t
    get_base_name_length_of_path(const char *path, size_t length) {
        return _get_base_name_length_of_path(path, length, length);
    }

    template <size_t N>
    static constexpr size_t get_base_name_length_of_path(const char (&path)[N]
    ) {
        return _get_base_name_length_of_path(path, N - 1, N - 1);
    }

    template <size_t N>
    static constexpr size_t
    get_length_of_constexpr_char_array(__attribute__((unused))
                                       const char (&path)[N]) {
        return N - 1;
    }

private:
    static const char _HEX_ALPHABET_TABLE[];
    static constexpr size_t _NUMBER_OF_LETTERS_OF_POINTER_IN_HEX =
        sizeof(uintptr_t) * 2;

    template <typename T, typename = enable_for_integer_types<T>>
    static constexpr int _log10_floor(T integer_value) {
        return integer_value < 10 ? 0 : (1 + _log10_floor(integer_value / 10));
    }

    static constexpr const char *
    _get_base_name_of_path(const char *path, size_t current_offset) {
        return current_offset <= 0
                   ? path
                   : (path[current_offset - 1] == '/'
                          ? path + current_offset
                          : _get_base_name_of_path(path, current_offset - 1));
    }

    static constexpr size_t _get_base_name_length_of_path(
        const char *path, size_t current_offset, const size_t total_length
    ) {
        return current_offset <= 0
                   ? total_length
                   : (path[current_offset - 1] == '/'
                          ? total_length - current_offset
                          : _get_base_name_length_of_path(
                              path, current_offset - 1, total_length
                          ));
    }
};

// type dispatcher
template <>
struct Format::is_one_of_8_integer_types<short> {
    static constexpr bool value = true;
};
template <>
struct Format::is_one_of_8_integer_types<int> {
    static constexpr bool value = true;
};
template <>
struct Format::is_one_of_8_integer_types<long> {
    static constexpr bool value = true;
};
template <>
struct Format::is_one_of_8_integer_types<long long> {
    static constexpr bool value = true;
};
template <>
struct Format::is_one_of_8_integer_types<unsigned short> {
    static constexpr bool value = true;
};
template <>
struct Format::is_one_of_8_integer_types<unsigned int> {
    static constexpr bool value = true;
};
template <>
struct Format::is_one_of_8_integer_types<unsigned long> {
    static constexpr bool value = true;
};
template <>
struct Format::is_one_of_8_integer_types<unsigned long long> {
    static constexpr bool value = true;
};

// `%.12g`: -1.23456789012e+308
template <>
struct Format::
    get_min_number_of_chars_required_to_represent_value_of_type<double, void> {
    static constexpr size_t value = 19;
};

} // namespace util

} // namespace xubinh_server

#endif