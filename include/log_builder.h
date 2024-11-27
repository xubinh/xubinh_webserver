#ifndef XUBINH_SERVER_LOG_BUILDER
#define XUBINH_SERVER_LOG_BUILDER

#include <cerrno>
#include <string.h>
#include <string>

#include "log_buffer.h"
#include "util/format.h"

namespace xubinh_server {

enum class LogLevel {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
    NUMBER_OF_ALL_LEVELS
};

class LogBuilder {
private:
    // not unique_ptr but stack directly
    LogEntryBuffer _entry_buffer;

    const char *_source_file_base_name;
    size_t _source_file_base_name_length = 0;

    const int _line_number;

    const char *_function_name;
    size_t _function_name_length = 0;

    const int _saved_errno;

    LogBuilder(
        LogLevel log_level,
        const char *source_file_base_name,
        int line_number,
        const char *function_name,
        int saved_errno
    );

    static LogLevel _log_level_mask;

    static constexpr char *
        _LOG_LEVEL_STRINGS[static_cast<size_t>(LogLevel::NUMBER_OF_ALL_LEVELS)]{
            "TRACE", "DEBUG", "INFO ", "WARN ", "ERROR", "FATAL"};

    static constexpr size_t _LENGTH_OF_LOG_LEVEL_STRINGS = 5;

public:
    // TRACE | DEBUG
    LogBuilder(
        LogLevel log_level,
        const char *source_file_base_name,
        int line_number,
        const char *function_name
    )
        : LogBuilder(
            log_level, source_file_base_name, line_number, function_name, 0
        ) {
    }

    // INFO | WARN | ERROR | FATAL
    LogBuilder(
        LogLevel log_level, const char *source_file_base_name, int line_number
    )
        : LogBuilder(
            log_level, source_file_base_name, line_number, nullptr, 0
        ) {
    }

    // SYS_ERROR | SYS_FATAL
    LogBuilder(
        LogLevel log_level,
        const char *source_file_base_name,
        int line_number,
        int saved_errno
    )
        : LogBuilder(
            log_level, source_file_base_name, line_number, nullptr, saved_errno
        ) {
    }

    ~LogBuilder();

private:
    // alias
    template <typename T>
    using enable_for_integer_types = util::Format::enable_for_integer_types<T>;

public:
    // declaration
    template <typename T, typename = enable_for_integer_types<T>>
    LogBuilder &operator<<(T integer);

    LogBuilder &operator<<(double floating_point);

    LogBuilder &operator<<(bool boolean_value) {
        _entry_buffer.append(boolean_value ? "true" : "false", 5);

        return *this;
    }

    LogBuilder &operator<<(const void *pointer);

    // LogBuilder &operator<<(signed char value);
    // LogBuilder &operator<<(unsigned char value);

    LogBuilder &operator<<(char character) {
        _entry_buffer.append(&character, sizeof(char));

        return *this;
    }

    LogBuilder &operator<<(const char *a_string) {
        if (a_string) {
            _entry_buffer.append(a_string, ::strlen(a_string));
        }

        return *this;
    }

    LogBuilder &operator<<(const std::string &a_string) {
        return *this << a_string.c_str();
    }

    static LogLevel get_log_level() {
        return _log_level_mask;
    }

    static void set_log_level(LogLevel log_level) {
        _log_level_mask = log_level;
    }
};

// explicit instantiation
extern template LogBuilder &LogBuilder::operator<<(short integer);
extern template LogBuilder &LogBuilder::operator<<(unsigned short integer);
extern template LogBuilder &LogBuilder::operator<<(int integer);
extern template LogBuilder &LogBuilder::operator<<(unsigned int integer);
extern template LogBuilder &LogBuilder::operator<<(long integer);
extern template LogBuilder &LogBuilder::operator<<(unsigned long integer);
extern template LogBuilder &LogBuilder::operator<<(long long integer);
extern template LogBuilder &LogBuilder::operator<<(unsigned long long integer);

} // namespace xubinh_server

#define ENABLE_TRACE                                                           \
    (xubinh_server::LogLevel::TRACE                                            \
     >= xubinh_server::LogBuilder::get_log_level())
#define ENABLE_DEBUG                                                           \
    (xubinh_server::LogLevel::DEBUG                                            \
     >= xubinh_server::LogBuilder::get_log_level())
#define ENABLE_INFO                                                            \
    (xubinh_server::LogLevel::INFO                                             \
     >= xubinh_server::LogBuilder::get_log_level())

#define LOG_TRACE                                                              \
    if (ENABLE_TRACE)                                                          \
    xubinh_server::LogBuilder(                                                 \
        xubinh_server::LogLevel::TRACE,                                        \
        xubinh_server::util::Format::get_base_name_of_path(__FILE__),          \
        __LINE__,                                                              \
        __FUNCTION__                                                           \
    )
#define LOG_DEBUG                                                              \
    if (ENABLE_DEBUG)                                                          \
    xubinh_server::LogBuilder(                                                 \
        xubinh_server::LogLevel::DEBUG,                                        \
        xubinh_server::util::Format::get_base_name_of_path(__FILE__),          \
        __LINE__,                                                              \
        __FUNCTION__                                                           \
    )
#define LOG_INFO                                                               \
    if (ENABLE_INFO)                                                           \
    xubinh_server::LogBuilder(                                                 \
        xubinh_server::LogLevel::INFO,                                         \
        xubinh_server::util::Format::get_base_name_of_path(__FILE__),          \
        __LINE__                                                               \
    )

#define LOG_WARN                                                               \
    xubinh_server::LogBuilder(                                                 \
        xubinh_server::LogLevel::WARN,                                         \
        xubinh_server::util::Format::get_base_name_of_path(__FILE__),          \
        __LINE__                                                               \
    )

#define LOG_ERROR                                                              \
    xubinh_server::LogBuilder(                                                 \
        xubinh_server::LogLevel::ERROR,                                        \
        xubinh_server::util::Format::get_base_name_of_path(__FILE__),          \
        __LINE__                                                               \
    )
#define LOG_FATAL                                                              \
    xubinh_server::LogBuilder(                                                 \
        xubinh_server::LogLevel::FATAL,                                        \
        xubinh_server::util::Format::get_base_name_of_path(__FILE__),          \
        __LINE__                                                               \
    )

#define LOG_SYS_ERROR                                                          \
    xubinh_server::LogBuilder(                                                 \
        xubinh_server::LogLevel::ERROR,                                        \
        xubinh_server::util::Format::get_base_name_of_path(__FILE__),          \
        __LINE__,                                                              \
        errno                                                                  \
    )
#define LOG_SYS_FATAL                                                          \
    xubinh_server::LogBuilder(                                                 \
        xubinh_server::LogLevel::FATAL,                                        \
        xubinh_server::util::Format::get_base_name_of_path(__FILE__),          \
        __LINE__,                                                              \
        errno                                                                  \
    )

#endif