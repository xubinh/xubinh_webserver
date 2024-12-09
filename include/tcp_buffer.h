#ifndef XUBINH_SERVER_TCP_BUFFER
#define XUBINH_SERVER_TCP_BUFFER

#include <cstddef>
#include <vector>

namespace xubinh_server {

// not thread-safe
class MutableSizeTcpBuffer {
public:
    // fixed-size write
    static void write_to_tcp_buffer(
        const char *source, MutableSizeTcpBuffer &destination, size_t data_size
    );

    // fixed-size read
    static void read_from_tcp_buffer(
        MutableSizeTcpBuffer &source, char *destination, size_t data_size
    );

    MutableSizeTcpBuffer(
        size_t initial_buffer_size = _DEFAULT_INITIAL_BUFFER_SIZE
    )
        // prevent UB produced by doing a `begin()` on an empty vector
        : _buffer(std::max(initial_buffer_size, static_cast<size_t>(1))) {

        // _buffer.resize(_buffer.capacity());
        // [NOTE]: size equals to capacity after initialization
    }

    char *begin() {
        return &(*_buffer.begin());
    }

    const char *begin() const {
        return &(*_buffer.begin());
    }

    char *current_read_position() {
        return begin() + _current_read_position;
    }

    const char *current_read_position() const {
        return begin() + _current_read_position;
    }

    char *current_write_position() {
        return begin() + _current_write_position;
    }

    const char *current_write_position() const {
        return begin() + _current_write_position;
    }

    char *end() {
        return &(*_buffer.end());
    }

    const char *end() const {
        return &(*_buffer.end());
    }

    size_t prependable_size() const {
        return _current_read_position;
    }

    size_t readable_size() const {
        return _current_write_position - _current_read_position;
    }

    size_t writable_size() const {
        return _buffer.size() - _current_write_position;
    }

    void forward_read_position(size_t size_of_read) {
        _current_read_position += size_of_read;

        if (_current_read_position > _current_write_position) {
            _current_read_position = _current_write_position;
        }
    }

    void forward_write_position(size_t size_of_written) {
        _current_write_position += size_of_written;

        if (_current_write_position > _buffer.size()) {
            _current_write_position == _buffer.size();
        }
    }

    void make_space(size_t size_of_space);

private:
    static constexpr size_t _DEFAULT_INITIAL_BUFFER_SIZE = 1024;

    void _move_data_to_front() {
        std::copy(
            current_read_position(), current_write_position(), _buffer.begin()
        );

        _current_write_position -= _current_read_position;

        _current_read_position = 0;
    }

    std::vector<char> _buffer;
    size_t _current_read_position{0};
    size_t _current_write_position{0};
};

} // namespace xubinh_server

#endif