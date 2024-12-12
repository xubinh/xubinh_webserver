#include "tcp_buffer.h"

namespace xubinh_server {

void MutableSizeTcpBuffer::make_space(size_t size_of_space) {
    // write
    if (size_of_space <= get_writable_size()) {
        return;
    }

    // copy + write
    if (size_of_space <= get_prependable_size() + get_writable_size()) {
        _move_data_to_front();

        return;
    }

    // reallocate + copy + write
    _buffer.resize(_current_write_offset + size_of_space);

    // calibrating `size()` to be the same as `capacity()`
    _buffer.resize(_buffer.capacity());
}

void MutableSizeTcpBuffer::write(const char *buffer, size_t buffer_size) {
    make_space(buffer_size);

    std::copy(buffer, buffer + buffer_size, get_current_write_position());

    forward_write_position(buffer_size);
}

} // namespace xubinh_server