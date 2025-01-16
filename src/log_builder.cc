#include "log_builder.h"
#include "log_collector.h"
#include "util/errno.h"
#include "util/format.h"
#include "util/this_thread.h"
#include "util/time_point.h"

namespace {

// alias
template <typename T>
constexpr size_t _get_number_of_chars() {
    return xubinh_server::util::Format::
        get_min_number_of_chars_required_to_represent_value_of_type<T>::value;
}

} // namespace

namespace xubinh_server {

LogBuilder::~LogBuilder() {
    if (_source_file_base_name) {
        _entry_buffer.append(" | ", 3);

        _entry_buffer.append(
            _source_file_base_name, _source_file_base_name_length
        );

        _entry_buffer.append(":", 1);

        *this << _line_number;
    }

    if (_function_name) {
        _entry_buffer.append(" | ", 3);

        _entry_buffer.append(_function_name, _function_name_length);
    }

    if (_saved_errno) {
        _entry_buffer.append(" | ", 3);

        *this << util::strerror_tl(_saved_errno);

        _entry_buffer.append(" (errno=", 8);

        *this << _saved_errno;

        _entry_buffer.append(")", 1);
    }

    _entry_buffer.append("\n", 1);

    LogCollector::get_instance().take_this_log(
        _entry_buffer.get_start_address_of_buffer(), _entry_buffer.length()
    );

    if (_log_level == LogLevel::FATAL) {
        LogCollector::get_instance().abort();
    }
}

// definition
template <typename T, typename = LogBuilder::enable_for_integer_types<T>>
LogBuilder &LogBuilder::operator<<(T integer) {
    if (_entry_buffer.length_of_spare() > _get_number_of_chars<T>()) {
        _entry_buffer.increment_length(
            util::Format::convert_integer_to_decimal_string(
                _entry_buffer.get_start_address_of_spare(), integer
            )
        );
    }

    return *this;
}

// explicit instantiation
template LogBuilder &LogBuilder::operator<<(short integer);
template LogBuilder &LogBuilder::operator<<(unsigned short integer);
template LogBuilder &LogBuilder::operator<<(int integer);
template LogBuilder &LogBuilder::operator<<(unsigned int integer);
template LogBuilder &LogBuilder::operator<<(long integer);
template LogBuilder &LogBuilder::operator<<(unsigned long integer);
template LogBuilder &LogBuilder::operator<<(long long integer);
template LogBuilder &LogBuilder::operator<<(unsigned long long integer);

LogBuilder &LogBuilder::operator<<(double floating_point) {
    if (_entry_buffer.length_of_spare() > _get_number_of_chars<double>()) {
        _entry_buffer.increment_length(::snprintf(
            _entry_buffer.get_start_address_of_spare(),
            _get_number_of_chars<double>(),
            "%.12g",
            floating_point
        ));
    }

    return *this;
}

LogBuilder &LogBuilder::operator<<(const void *pointer) {
    if (_entry_buffer.length_of_spare() > _get_number_of_chars<void *>()) {
        util::Format::convert_pointer_to_hex_string(
            _entry_buffer.get_start_address_of_spare(), pointer
        );

        _entry_buffer.increment_length(_get_number_of_chars<void *>());
    }

    return *this;
}

LogLevel LogBuilder::_log_level_mask = LogLevel::INFO;

const char *LogBuilder::_LOG_LEVEL_STRINGS[static_cast<size_t>(
    LogLevel::NUMBER_OF_ALL_LEVELS
)]{"TRACE", "DEBUG", "INFO ", "WARN ", "ERROR", "FATAL"};

LogBuilder::LogBuilder(
    LogLevel log_level,
    const char *source_file_base_name,
    size_t source_file_base_name_length,
    int line_number,
    const char *function_name,
    size_t function_name_length,
    int saved_errno
)
    : _log_level(log_level)
    , _source_file_base_name(source_file_base_name)
    , _source_file_base_name_length(source_file_base_name_length)
    , _line_number(line_number)
    , _function_name(function_name)
    , _function_name_length(function_name_length)
    , _saved_errno(saved_errno) {

    std::string datetime_string =
        util::TimePoint::get_datetime_string(util::TimePoint::Purpose::PRINTING
        );

    _entry_buffer.append(datetime_string.c_str(), datetime_string.size());

    _entry_buffer.append(" | ", 3);

    _entry_buffer.append(
        util::this_thread::get_tid_string(),
        util::this_thread::get_tid_string_length()
    );

    _entry_buffer.append(" | ", 3);

    _entry_buffer.append(
        _LOG_LEVEL_STRINGS[static_cast<size_t>(log_level)],
        _LENGTH_OF_LOG_LEVEL_STRINGS
    );

    _entry_buffer.append(" | ", 3);
}

} // namespace xubinh_server
