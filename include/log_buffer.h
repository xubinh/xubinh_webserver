#ifndef XUBINH_SERVER_LOG_BUFFER
#define XUBINH_SERVER_LOG_BUFFER

#include <cstddef>
#include <type_traits>

namespace xubinh_server {

static constexpr size_t LOG_ENTRY_BUFFER_SIZE = 4 * 1000; // 4 KB 的普通缓冲区
static constexpr size_t LOG_CHUNK_BUFFER_SIZE =
    4 * 1000 * 1000; // 4 MB 的大缓冲区

// declaration
template <size_t LOG_BUFFER_SIZE>
class LogBuffer {
private:
    char _buffer[LOG_BUFFER_SIZE];
    const char *_end_address_of_buffer = _buffer + LOG_BUFFER_SIZE - 1; // `\0`
    char *_start_address_of_spare = _buffer;

    static constexpr size_t _MAX_BUFFER_SIZE = LOG_BUFFER_SIZE;

public:
    // 无需拷贝, 想要新的缓冲区直接创建即可
    LogBuffer(const LogBuffer &) = delete;
    LogBuffer &operator=(const LogBuffer &) = delete;

    // 因为总是配合 `std::unique_ptr` 进行移动, 无需移动对象本身,
    // 因此定义为删除的
    LogBuffer(const LogBuffer &&) = delete;
    LogBuffer &operator=(const LogBuffer &&) = delete;

    LogBuffer() = default;

    ~LogBuffer() = default;

    const char *get_start_address_of_buffer() const {
        return _buffer;
    }

    char *get_start_address_of_spare() {
        return _start_address_of_spare;
    }

    size_t length() {
        return static_cast<size_t>(_start_address_of_spare - _buffer);
    }

    size_t increment_length(size_t delta) {
        return _start_address_of_spare += delta;
    }

    size_t length_of_spare() {
        return static_cast<size_t>(
            _end_address_of_buffer - _start_address_of_spare
        );
    }

    void append(const char *data, size_t data_size);

    void reset() {
        _start_address_of_spare = _buffer;
    }

    static constexpr size_t capacity() const {
        return _MAX_BUFFER_SIZE;
    }
};

// helper declarations
using LogEntryBuffer = LogBuffer<LOG_ENTRY_BUFFER_SIZE>;
using LogChunkBuffer = LogBuffer<LOG_CHUNK_BUFFER_SIZE>;

} // namespace xubinh_server

#endif