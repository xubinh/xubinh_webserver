#ifndef __XUBINH_SERVER_TCP_BUFFER
#define __XUBINH_SERVER_TCP_BUFFER

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <vector>

namespace xubinh_server {

// not thread-safe
class MutableSizeTcpBuffer {
public:
    MutableSizeTcpBuffer(
        size_t initial_buffer_size = _DEFAULT_INITIAL_BUFFER_SIZE
    )
        // prevent UB produced by doing a `begin()` on an empty vector
        : _buffer(std::max(initial_buffer_size, static_cast<size_t>(1))) {

        // _buffer.resize(_buffer.capacity());
        // [NOTE]: size equals to capacity after initialization
    }

    char *get_current_read_position() {
        return _begin() + _current_read_offset;
    }

    const char *get_current_read_position() const {
        return _begin() + _current_read_offset;
    }

    char *get_current_write_position() {
        return _begin() + _current_write_offset;
    }

    const char *get_current_write_position() const {
        return _begin() + _current_write_offset;
    }

    size_t get_prependable_size() const {
        return _current_read_offset;
    }

    size_t get_readable_size() const {
        return _current_write_offset - _current_read_offset;
    }

    size_t get_writable_size() const {
        return _buffer.size() - _current_write_offset;
    }

    void forward_read_position(size_t size_of_read) {
        _current_read_offset += size_of_read;

        if (_current_read_offset > _current_write_offset) {
            _current_read_offset = _current_write_offset;
        }
    }

    void forward_write_position(size_t size_of_written) {
        _current_write_offset += size_of_written;

        if (_current_write_offset > _buffer.size()) {
            _current_write_offset == _buffer.size();
        }
    }

    void make_space(size_t size_of_space);

    // fixed-size write
    void write(const char *buffer, size_t buffer_size);

    const char *get_next_newline_position() {
        return static_cast<const char *>(
            ::memchr(get_current_read_position(), '\n', get_readable_size())
        );
    }

    const char *get_next_crlf_position() {
        auto next_crlf_position = std::search(
            get_current_read_position(),
            get_current_write_position(),
            CRLF,
            CRLF + 2
        );

        return next_crlf_position == get_current_write_position()
                   ? nullptr
                   : next_crlf_position;
    }

    void write_space() {
        write(" ", 1);
    }

    void write_newline() {
        write("\n", 1);
    }

    void write_crlf() {
        write("\r\n", 2);
    }

    void write_colon() {
        write(":", 1);
    }

private:
    static constexpr size_t _DEFAULT_INITIAL_BUFFER_SIZE = 1024;

    static char CRLF[];

    char *_begin() {
        return &(*_buffer.begin());
    }

    const char *_begin() const {
        return &(*_buffer.begin());
    }

    char *_end() {
        return &(*_buffer.end());
    }

    const char *_end() const {
        return &(*_buffer.end());
    }

    void _move_data_to_front() {
        std::copy(
            get_current_read_position(),
            get_current_write_position(),
            _buffer.begin()
        );

        _current_write_offset -= _current_read_offset;

        _current_read_offset = 0;
    }

    std::vector<char> _buffer;
    size_t _current_read_offset{0};
    size_t _current_write_offset{0};
};

} // namespace xubinh_server

#endif