#ifndef XUBINH_SERVER_LOG_BUILDER
#define XUBINH_SERVER_LOG_BUILDER

#include <cerrno>
#include <string.h>
#include <string>

#include "log_buffer.h"
#include "utils/format.h"

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
        const char *source_file_name,
        int line_number,
        const char *function_name,
        int saved_errno
    );

    static LogLevel _log_level_mask;

public:
    // TRACE | DEBUG
    LogBuilder(
        LogLevel log_level,
        const char *source_file_name,
        int line_number,
        const char *function_name
    )
        : LogBuilder(
            log_level, source_file_name, line_number, function_name, 0
        ) {
    }

    // INFO | WARN | ERROR | FATAL
    LogBuilder(
        LogLevel log_level, const char *source_file_name, int line_number
    )
        : LogBuilder(log_level, source_file_name, line_number, nullptr, 0) {
    }

    // SYS_ERROR | SYS_FATAL
    LogBuilder(
        LogLevel log_level,
        const char *source_file_name,
        int line_number,
        int saved_errno
    )
        : LogBuilder(
            log_level, source_file_name, line_number, nullptr, saved_errno
        ) {
    }

    ~LogBuilder();

    // has to be non-template since the type information is lost when caching
    // the numbers of digits as static members
    LogBuilder &operator<<(short integer);
    LogBuilder &operator<<(unsigned short integer);
    LogBuilder &operator<<(int integer);
    LogBuilder &operator<<(unsigned int integer);
    LogBuilder &operator<<(long integer);
    LogBuilder &operator<<(unsigned long integer);
    LogBuilder &operator<<(long long integer);
    LogBuilder &operator<<(unsigned long long integer);

    LogBuilder &operator<<(float floating_point) {
        return *this << static_cast<double>(floating_point);
    }

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

    static int NUMBER_OF_DIGITS_OF_TYPE_SHORT;
    static int NUMBER_OF_DIGITS_OF_TYPE_UNSIGNED_SHORT;
    static int NUMBER_OF_DIGITS_OF_TYPE_INT;
    static int NUMBER_OF_DIGITS_OF_TYPE_UNSIGNED_INT;
    static int NUMBER_OF_DIGITS_OF_TYPE_LONG;
    static int NUMBER_OF_DIGITS_OF_TYPE_UNSIGNED_LONG;
    static int NUMBER_OF_DIGITS_OF_TYPE_LONG_LONG;
    static int NUMBER_OF_DIGITS_OF_TYPE_UNSIGNED_LONG_LONG;

    static int NUMBER_OF_DIGITS_OF_TYPE_DOUBLE;

    static int NUMBER_OF_DIGITS_OF_TYPE_POINTER;

    static LogLevel get_log_level() {
        return _log_level_mask;
    }

    static void set_log_level(LogLevel log_level) {
        _log_level_mask = log_level;
    }
};

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
    LogBuilder(xubinh_server::LogLevel::TRACE, __FILE__, __LINE__, __FUNCTION__)
#define LOG_DEBUG                                                              \
    if (ENABLE_DEBUG)                                                          \
    LogBuilder(xubinh_server::LogLevel::DEBUG, __FILE__, __LINE__, __FUNCTION__)
#define LOG_INFO                                                               \
    if (ENABLE_INFO)                                                           \
    LogBuilder(xubinh_server::LogLevel::INFO, __FILE__, __LINE__)

#define LOG_WARN LogBuilder(xubinh_server::LogLevel::WARN, __FILE__, __LINE__)

#define LOG_ERROR LogBuilder(xubinh_server::LogLevel::ERROR, __FILE__, __LINE__)
#define LOG_FATAL LogBuilder(xubinh_server::LogLevel::FATAL, __FILE__, __LINE__)

#define LOG_SYS_ERROR                                                          \
    LogBuilder(xubinh_server::LogLevel::ERROR, __FILE__, __LINE__, errno)
#define LOG_SYS_FATAL                                                          \
    LogBuilder(xubinh_server::LogLevel::FATAL, __FILE__, __LINE__, errno)

} // namespace xubinh_server

#endif