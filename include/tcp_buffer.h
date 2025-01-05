#ifndef __XUBINH_SERVER_TCP_BUFFER
#define __XUBINH_SERVER_TCP_BUFFER

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

namespace xubinh_server {

// not thread-safe
class MutableSizeTcpBuffer {
public:
    MutableSizeTcpBuffer() noexcept {
        _volatile_buffer_begin_ptr = const_cast<char *>(_buffer.c_str());
    }

    const char *get_read_position() const {
        return _volatile_buffer_begin_ptr + _read_offset;
    }

    size_t get_readable_size() const {
        return _write_offset - _read_offset;
    }

    // does not check internally; make sure to use it well
    void forward_read_position(size_t number_of_bytes_read) {
        _read_offset =
            std::min(_read_offset + number_of_bytes_read, _write_offset);
    }

    const char *get_next_newline_position();

    const char *get_next_crlf_position();

    // appends size-known external (i.e. already existed) buffer of data into
    // this TCP buffer
    void append(const char *external_buffer, size_t external_buffer_size) {
        auto internal_buffer = _make_space(external_buffer_size);

        ::memcpy(internal_buffer, external_buffer, external_buffer_size);
    }

    void append(const std::string &external_buffer) {
        append(external_buffer.c_str(), external_buffer.size());
    }

    void append_space() {
        append(" ", 1);
    }

    void append_newline() {
        append("\n", 1);
    }

    void append_crlf() {
        append("\r\n", 2);
    }

    void append_colon() {
        append(":", 1);
    }

private:
    // makes space (and possibly reallocates memory), default initializes with
    // `\0`, and return the begin address of the newly made space (the size of
    // which is exactly the size that passed in)
    //
    // - the buffer size will always be added by the size of the newly made
    // space, whether or not any data is written into it
    // - returns `nullptr` if the size passed in is zero
    char *_make_space(size_t size);

    std::string _buffer;
    char *_volatile_buffer_begin_ptr;
    size_t _read_offset{0};
    size_t _write_offset{0};
};

} // namespace xubinh_server

#endif