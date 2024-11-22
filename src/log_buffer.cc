#include <cstring> // std::memcpy

#include "../include/log_buffer.h"

namespace xubinh_server {

template <size_t LOG_BUFFER_SIZE>
LogBuffer<LOG_BUFFER_SIZE>::LogBuffer() = default;

template <size_t LOG_BUFFER_SIZE>
LogBuffer<LOG_BUFFER_SIZE>::~LogBuffer() = default;

template <size_t LOG_BUFFER_SIZE>
const char *LogBuffer<LOG_BUFFER_SIZE>::get_begin_address_of_buffer() const {
    return _buffer;
}

template <size_t LOG_BUFFER_SIZE>
void LogBuffer<LOG_BUFFER_SIZE>::append(
    const char *data, std::size_t data_size
) {
    // 如果大小不够则直接丢弃:
    if (data_size > get_size_of_available_area()) {
        return;
    }

    std::memcpy(_begin_position_of_available_area, data, data_size);

    _begin_position_of_available_area += data_size;
}

// explicit instantiation:
template class LogBuffer<LOG_ENTRY_BUFFER_SIZE>;
template class LogBuffer<LOG_CHUNK_BUFFER_SIZE>;

} // namespace xubinh_server