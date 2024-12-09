#include "tcp_buffer.h"

namespace xubinh_server {

void MutableSizeTcpBuffer::write_to_tcp_buffer(
    const char *source, MutableSizeTcpBuffer &destination, size_t data_size
) {
    destination.make_space(data_size);

    std::copy(source, source + data_size, destination.current_write_position());

    destination.forward_write_position(data_size);
}

void MutableSizeTcpBuffer::read_from_tcp_buffer(
    MutableSizeTcpBuffer &source, char *destination, size_t data_size
) {
    std::copy(
        source.current_read_position(),
        source.current_read_position() + data_size,
        destination
    );

    source.forward_read_position(data_size);
}

void MutableSizeTcpBuffer::make_space(size_t size_of_space) {
    // write
    if (size_of_space <= writable_size()) {
        return;
    }

    // copy + write
    if (size_of_space <= prependable_size() + writable_size()) {
        _move_data_to_front();

        return;
    }

    // reallocate + copy + write
    _buffer.resize(_current_write_position + size_of_space);

    // calibrating `size()` to be the same as `capacity()`
    _buffer.resize(_buffer.capacity());
}

} // namespace xubinh_server