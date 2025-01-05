#include "tcp_buffer.h"

namespace xubinh_server {

const char *MutableSizeTcpBuffer::get_next_newline_position() {
    if (__builtin_expect(_read_offset >= _write_offset, false)) {
        return nullptr;
    }

    // [TODO]: optimize this, e.g. starts from where previously left
    for (auto i = _read_offset; i < _write_offset; i++) {
        if (_volatile_buffer_begin_ptr[i] == '\n') {
            return _volatile_buffer_begin_ptr + i;
        }
    }

    return nullptr;
}

const char *MutableSizeTcpBuffer::get_next_crlf_position() {
    if (__builtin_expect(_read_offset >= _write_offset, false)) {
        return nullptr;
    }

    // [TODO]: optimize this, e.g. starts from where previously left
    for (auto i = _read_offset; i < _write_offset - 1; i++) {
        if (_volatile_buffer_begin_ptr[i] == '\r'
            && _volatile_buffer_begin_ptr[i + 1] == '\n') {

            return _volatile_buffer_begin_ptr + i;
        }
    }

    return nullptr;
}

char *MutableSizeTcpBuffer::_make_space(size_t size) {
    if (__builtin_expect(size == 0, false)) {
        return nullptr;
    }

    auto directly_extendable_size = _buffer.capacity() - _write_offset;

    // if extendable at the end directly
    if (size <= directly_extendable_size) {
        _buffer.resize(_write_offset + size);

        auto end_ptr_before_extension =
            _volatile_buffer_begin_ptr + _write_offset;

        _write_offset += size; // extension

        return end_ptr_before_extension;
    }

    // if extendable at the end after compaction
    else if (size <= directly_extendable_size + _read_offset) {
        auto readable_size = _write_offset - _read_offset;

        ::memcpy(
            _volatile_buffer_begin_ptr,
            _volatile_buffer_begin_ptr + _read_offset,
            readable_size
        ); // compaction

        _read_offset = 0;

        _buffer.resize(readable_size + size); // extension

        auto end_ptr_before_extension =
            _volatile_buffer_begin_ptr + readable_size;

        return end_ptr_before_extension;
    }

    // otherwise reallocates new memory buffer and skip compaction till the next
    // time
    else {
        _buffer.resize(_write_offset + size); // reallocation & copying

        _volatile_buffer_begin_ptr =
            const_cast<char *>(_buffer.c_str()); // calibration

        auto end_ptr_before_extension =
            _volatile_buffer_begin_ptr + _write_offset;

        _write_offset += size; // extension

        return end_ptr_before_extension;
    }
}

} // namespace xubinh_server