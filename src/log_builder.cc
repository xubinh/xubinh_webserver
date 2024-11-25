#include "log_builder.h"
#include "log_collector.h"
#include "utils/current_thread.h"
#include "utils/datetime.h"
#include "utils/errno.h"
#include "utils/format.h"

namespace xubinh_server {

namespace {

constexpr char
    *_LOG_LEVEL_STRINGS[static_cast<size_t>(LogLevel::NUMBER_OF_ALL_LEVELS)]{
        "TRACE", "DEBUG", "INFO ", "WARN ", "ERROR", "FATAL"};
constexpr size_t _LENGTH_OF_LOG_LEVEL_STRINGS = 5;

} // namespace

LogLevel LogBuilder::_log_level_mask = LogLevel::INFO;

LogBuilder::LogBuilder(
    LogLevel log_level,
    const char *source_file_name, // [TODO]: compile-time base name getter
    int line_number,
    const char *function_name,
    int saved_errno
)
    : _source_file_base_name(source_file_name), _line_number(line_number),
      _function_name(function_name), _saved_errno(saved_errno) {

    if (source_file_name) {
        _source_file_base_name_length = strlen(source_file_name);
    }

    if (function_name) {
        _function_name_length = strlen(function_name);
    }

    _entry_buffer.append(
        utils::Datetime::get_datetime_string(utils::DatetimePurpose::PRINTING)
            .c_str(),
        utils::Datetime::DATETIME_STRING_LENGTHES
            [static_cast<size_t>(utils::DatetimePurpose::PRINTING)]
    );

    _entry_buffer.append(" | ", 3);

    _entry_buffer.append(
        utils::CurrentThread::get_tid_string(),
        utils::CurrentThread::get_tid_string_length()
    );

    _entry_buffer.append(" | ", 3);

    _entry_buffer.append(
        _LOG_LEVEL_STRINGS[static_cast<size_t>(log_level)],
        _LENGTH_OF_LOG_LEVEL_STRINGS
    );

    _entry_buffer.append(" | ", 3);
}

LogBuilder::~LogBuilder() {
    if (_source_file_base_name) {
        _entry_buffer.append(" | ", 3);

        _entry_buffer.append(
            _source_file_base_name, _source_file_base_name_length
        );
    }

    _entry_buffer.append(" | ", 3);

    *this << _line_number;

    if (_function_name) {
        _entry_buffer.append(" | ", 3);

        _entry_buffer.append(_function_name, _function_name_length);
    }

    if (_saved_errno) {
        _entry_buffer.append(" | ", 3);

        *this << utils::strerror_tl(_saved_errno);

        _entry_buffer.append(" (errno=", 8);

        *this << _saved_errno;

        _entry_buffer.append(")", 1);
    }

    _entry_buffer.append("\n", 1);

    LogCollector::get_instance().take_this_log(
        _entry_buffer.get_start_address_of_buffer(), _entry_buffer.length()
    );
}

LogBuilder &LogBuilder::operator<<(short integer) {
    if (_entry_buffer.length_of_spare()
        > LogBuilder::NUMBER_OF_DIGITS_OF_TYPE_SHORT) {
        _entry_buffer.increment_length(
            utils::Format::convert_integer_to_string_base_10(
                _entry_buffer.get_start_address_of_spare(), integer
            )
        );
    }

    return *this;
}

LogBuilder &LogBuilder::operator<<(unsigned short integer) {
    if (_entry_buffer.length_of_spare()
        > LogBuilder::NUMBER_OF_DIGITS_OF_TYPE_UNSIGNED_SHORT) {
        _entry_buffer.increment_length(
            utils::Format::convert_integer_to_string_base_10(
                _entry_buffer.get_start_address_of_spare(), integer
            )
        );
    }

    return *this;
}

LogBuilder &LogBuilder::operator<<(int integer) {
    if (_entry_buffer.length_of_spare()
        > LogBuilder::NUMBER_OF_DIGITS_OF_TYPE_INT) {
        _entry_buffer.increment_length(
            utils::Format::convert_integer_to_string_base_10(
                _entry_buffer.get_start_address_of_spare(), integer
            )
        );
    }

    return *this;
}

LogBuilder &LogBuilder::operator<<(unsigned int integer) {
    if (_entry_buffer.length_of_spare()
        > LogBuilder::NUMBER_OF_DIGITS_OF_TYPE_UNSIGNED_INT) {
        _entry_buffer.increment_length(
            utils::Format::convert_integer_to_string_base_10(
                _entry_buffer.get_start_address_of_spare(), integer
            )
        );
    }

    return *this;
}

LogBuilder &LogBuilder::operator<<(long integer) {
    if (_entry_buffer.length_of_spare()
        > LogBuilder::NUMBER_OF_DIGITS_OF_TYPE_LONG) {
        _entry_buffer.increment_length(
            utils::Format::convert_integer_to_string_base_10(
                _entry_buffer.get_start_address_of_spare(), integer
            )
        );
    }

    return *this;
}

LogBuilder &LogBuilder::operator<<(unsigned long integer) {
    if (_entry_buffer.length_of_spare()
        > LogBuilder::NUMBER_OF_DIGITS_OF_TYPE_UNSIGNED_LONG) {
        _entry_buffer.increment_length(
            utils::Format::convert_integer_to_string_base_10(
                _entry_buffer.get_start_address_of_spare(), integer
            )
        );
    }

    return *this;
}

LogBuilder &LogBuilder::operator<<(long long integer) {
    if (_entry_buffer.length_of_spare()
        > LogBuilder::NUMBER_OF_DIGITS_OF_TYPE_LONG_LONG) {
        _entry_buffer.increment_length(
            utils::Format::convert_integer_to_string_base_10(
                _entry_buffer.get_start_address_of_spare(), integer
            )
        );
    }

    return *this;
}

LogBuilder &LogBuilder::operator<<(unsigned long long integer) {
    if (_entry_buffer.length_of_spare()
        > LogBuilder::NUMBER_OF_DIGITS_OF_TYPE_UNSIGNED_LONG_LONG) {
        _entry_buffer.increment_length(
            utils::Format::convert_integer_to_string_base_10(
                _entry_buffer.get_start_address_of_spare(), integer
            )
        );
    }

    return *this;
}

LogBuilder &LogBuilder::operator<<(double floating_point) {
    if (_entry_buffer.length_of_spare()
        > LogBuilder::NUMBER_OF_DIGITS_OF_TYPE_DOUBLE) {
        _entry_buffer.increment_length(snprintf(
            _entry_buffer.get_start_address_of_spare(),
            NUMBER_OF_DIGITS_OF_TYPE_DOUBLE,
            "%.12g",
            floating_point
        ));
    }

    return *this;
}

LogBuilder &LogBuilder::operator<<(const void *pointer) {
    if (_entry_buffer.length_of_spare()
        > LogBuilder::NUMBER_OF_DIGITS_OF_TYPE_POINTER) {
        utils::Format::convert_pointer_to_hex_string(
            _entry_buffer.get_start_address_of_spare(), pointer
        );

        _entry_buffer.increment_length(
            LogBuilder::NUMBER_OF_DIGITS_OF_TYPE_POINTER
        );
    }

    return *this;
}

int LogBuilder::NUMBER_OF_DIGITS_OF_TYPE_SHORT =
    utils::Format::get_number_of_digits_of_type<short>();
int LogBuilder::NUMBER_OF_DIGITS_OF_TYPE_UNSIGNED_SHORT =
    utils::Format::get_number_of_digits_of_type<unsigned short>();
int LogBuilder::NUMBER_OF_DIGITS_OF_TYPE_INT =
    utils::Format::get_number_of_digits_of_type<int>();
int LogBuilder::NUMBER_OF_DIGITS_OF_TYPE_UNSIGNED_INT =
    utils::Format::get_number_of_digits_of_type<unsigned int>();
int LogBuilder::NUMBER_OF_DIGITS_OF_TYPE_LONG =
    utils::Format::get_number_of_digits_of_type<long>();
int LogBuilder::NUMBER_OF_DIGITS_OF_TYPE_UNSIGNED_LONG =
    utils::Format::get_number_of_digits_of_type<unsigned long>();
int LogBuilder::NUMBER_OF_DIGITS_OF_TYPE_LONG_LONG =
    utils::Format::get_number_of_digits_of_type<long long>();
int LogBuilder::NUMBER_OF_DIGITS_OF_TYPE_UNSIGNED_LONG_LONG =
    utils::Format::get_number_of_digits_of_type<unsigned long long>();

// `%.12g`: -1.23456789012e+308
int LogBuilder::NUMBER_OF_DIGITS_OF_TYPE_DOUBLE = 19;

int LogBuilder::NUMBER_OF_DIGITS_OF_TYPE_POINTER =
    sizeof(uintptr_t) * 2 + 2; // `0x` prefix

} // namespace xubinh_server