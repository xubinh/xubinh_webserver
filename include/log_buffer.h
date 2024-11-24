#ifndef XUBINH_SERVER_LOG_BUFFER
#define XUBINH_SERVER_LOG_BUFFER

#include <cstddef>
#include <type_traits>

namespace xubinh_server {

static constexpr std::size_t LOG_ENTRY_BUFFER_SIZE =
    4 * 1000; // 4 KB 的普通缓冲区
static constexpr std::size_t LOG_CHUNK_BUFFER_SIZE =
    4 * 1000 * 1000; // 4 MB 的大缓冲区

template <size_t LOG_BUFFER_SIZE>
class LogBuffer {
    // static_assert(
    //     LOG_BUFFER_SIZE == LOG_ENTRY_BUFFER_SIZE
    //         || LOG_BUFFER_SIZE == LOG_CHUNK_BUFFER_SIZE,
    //     "Invalid log buffer size"
    // );
    // 将模板分离编译就已经足以用来限制模板实参的取值了

private:
    char _buffer[LOG_BUFFER_SIZE];
    char *_begin_position_of_available_area = _buffer;
    const char *_end_position_of_buffer = _buffer + LOG_BUFFER_SIZE;

    static constexpr size_t _MAX_BUFFER_SIZE = LOG_BUFFER_SIZE;

public:
    LogBuffer();

    ~LogBuffer();

    // 无需拷贝, 想要新的缓冲区直接创建即可
    LogBuffer(const LogBuffer &) = delete;
    LogBuffer &operator=(const LogBuffer &) = delete;

    // 因为总是配合 `std::unique_ptr` 进行移动, 无需移动对象本身,
    // 因此定义为删除的
    LogBuffer(const LogBuffer &&) = delete;
    LogBuffer &operator=(const LogBuffer &&) = delete;

    const char *get_begin_address_of_buffer() const;

    std::size_t get_size_of_written_area() {
        return static_cast<size_t>(_begin_position_of_available_area - _buffer);
    }

    std::size_t get_size_of_available_area() {
        return static_cast<size_t>(
            _end_position_of_buffer - _begin_position_of_available_area
        );
    }

    void append(const char *data, std::size_t data_size);

    void reset() {
        _begin_position_of_available_area = _buffer;
    }

    static constexpr size_t capacity() const {
        return _MAX_BUFFER_SIZE;
    }
};

// explicit instantiation
extern template class LogBuffer<LOG_ENTRY_BUFFER_SIZE>;
extern template class LogBuffer<LOG_CHUNK_BUFFER_SIZE>;

using LogEntryBuffer = LogBuffer<LOG_ENTRY_BUFFER_SIZE>;
using LogChunkBuffer = LogBuffer<LOG_CHUNK_BUFFER_SIZE>;

} // namespace xubinh_server

#endif