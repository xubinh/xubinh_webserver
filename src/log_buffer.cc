#include <cstring> // std::memcpy

#include "log_buffer.h"

namespace xubinh_server {

template <size_t LOG_BUFFER_SIZE>
void LogBuffer<LOG_BUFFER_SIZE>::append(
    const char *data, std::size_t data_size
) {
    // 如果大小不够则直接丢弃:
    if (data_size > length_of_spare()) {
        return;
    }

    std::memcpy(_start_address_of_spare, data, data_size);

    _start_address_of_spare += data_size;
}

// explicit instantiation
template class LogBuffer<LOG_ENTRY_BUFFER_SIZE>;
template class LogBuffer<LOG_CHUNK_BUFFER_SIZE>;

} // namespace xubinh_server