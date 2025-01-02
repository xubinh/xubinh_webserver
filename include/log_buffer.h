#ifndef __XUBINH_SERVER_LOG_BUFFER
#define __XUBINH_SERVER_LOG_BUFFER

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <type_traits>

#include "util/type_traits.h"

namespace xubinh_server {

struct LogBufferSizeConfig {
    static constexpr size_t LOG_ENTRY_BUFFER_SIZE = 4 * 1000;        // 4 KB
    static constexpr size_t LOG_CHUNK_BUFFER_SIZE = 4 * 1000 * 1000; // 4 MB

    // value dispatcher
    template <size_t LOG_BUFFER_SIZE>
    struct is_valid_log_buffer_size
        : std::integral_constant<
              bool,
              LOG_BUFFER_SIZE == LOG_ENTRY_BUFFER_SIZE
                  || LOG_BUFFER_SIZE == LOG_CHUNK_BUFFER_SIZE> {};

    // SFINAE type alias
    template <size_t LOG_BUFFER_SIZE>
    using enable_if_is_valid_log_buffer_size_t = util::type_traits::enable_if_t<
        is_valid_log_buffer_size<LOG_BUFFER_SIZE>::value>;
};

// declaration
template <
    size_t LOG_BUFFER_SIZE,
    typename = LogBufferSizeConfig::enable_if_is_valid_log_buffer_size_t<
        LOG_BUFFER_SIZE>>
class FixedSizeLogBuffer {
public:
    static constexpr size_t capacity() {
        return _MAX_BUFFER_SIZE;
    }

    FixedSizeLogBuffer() = default;

    // no copy, just create new ones if wanted more
    FixedSizeLogBuffer(const FixedSizeLogBuffer &) = delete;
    FixedSizeLogBuffer &operator=(const FixedSizeLogBuffer &) = delete;

    // no move, should be used in conjunction with `std::unique_ptr`
    FixedSizeLogBuffer(FixedSizeLogBuffer &&) = delete;
    FixedSizeLogBuffer &operator=(FixedSizeLogBuffer &&) = delete;

    ~FixedSizeLogBuffer() = default;
    // ~FixedSizeLogBuffer() {
    //     if (LOG_BUFFER_SIZE == LogBufferSizeConfig::LOG_CHUNK_BUFFER_SIZE) {
    //         ::fprintf(stderr, "exit destructor of FixedSizeLogBuffer\n");
    //     }
    // }

    const char *get_start_address_of_buffer() const {
        return _buffer;
    }

    char *get_start_address_of_spare() {
        return _start_address_of_spare;
    }

    size_t length() {
        return static_cast<size_t>(_start_address_of_spare - _buffer);
    }

    void increment_length(size_t delta) {
        _start_address_of_spare += delta;
    }

    size_t length_of_spare() {
        return static_cast<size_t>(
            _end_address_of_buffer - _start_address_of_spare
        );
    }

    void append(const char *data, size_t data_size) {
        // throw away if no available space
        if (data_size > length_of_spare()) {
            return;
        }

        memcpy(_start_address_of_spare, data, data_size);

        _start_address_of_spare += data_size;
    }

    void reset() {
        _start_address_of_spare = _buffer;
    }

private:
    static constexpr size_t _MAX_BUFFER_SIZE = LOG_BUFFER_SIZE;

    char _buffer[LOG_BUFFER_SIZE];
    const char *_end_address_of_buffer = _buffer + LOG_BUFFER_SIZE - 1; // `\0`
    char *_start_address_of_spare = _buffer;
};

// explicit instantiation
extern template class FixedSizeLogBuffer<
    LogBufferSizeConfig::LOG_ENTRY_BUFFER_SIZE>;
extern template class FixedSizeLogBuffer<
    LogBufferSizeConfig::LOG_CHUNK_BUFFER_SIZE>;

// helper declaration
using LogEntryBuffer =
    FixedSizeLogBuffer<LogBufferSizeConfig::LOG_ENTRY_BUFFER_SIZE>;
using LogChunkBuffer =
    FixedSizeLogBuffer<LogBufferSizeConfig::LOG_CHUNK_BUFFER_SIZE>;

} // namespace xubinh_server

#endif