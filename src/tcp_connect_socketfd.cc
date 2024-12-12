#include <sys/socket.h>
#include <sys/types.h>

#include "log_builder.h"
#include "tcp_connect_socketfd.h"
#include "util/errno.h"
#include "util/time_point.h"

namespace xubinh_server {

TcpConnectSocketfd::TcpConnectSocketfd(
    int fd, EventLoop *event_loop, const std::string &id
)
    : _id(id), _pollable_file_descriptor(fd, event_loop) {

    if (fd < 0) {
        LOG_SYS_FATAL << "failed creating eventfd";
    }

    _pollable_file_descriptor.register_read_event_callback(
        std::bind(_read_event_callback, this)
    );

    _pollable_file_descriptor.register_write_event_callback(
        std::bind(_write_event_callback, this)
    );

    _pollable_file_descriptor.register_close_event_callback(
        std::bind(_close_event_callback, this)
    );

    _pollable_file_descriptor.register_error_event_callback(
        std::bind(_error_event_callback, this)
    );

    // TCP connections must ensure lifetime safety, since they could be
    // destroyed by themselves or by the callbacks registered in them
    _pollable_file_descriptor.register_weak_lifetime_guard(shared_from_this());
}

void TcpConnectSocketfd::send(const char *data, size_t data_size) {
    // leave the writing to the event callback if already started listening
    if (_is_writing) {
        _output_buffer.write(data, data_size);

        return;
    }

    // otherwise try sending the data right now
    auto number_of_bytes_sent = _send_as_many_data(data, data_size);

    // no need for further writing if all data is sent
    if (number_of_bytes_sent == data_size) {
        if (_write_complete_callback) {
            _write_complete_callback(this);
        }

        return;
    }

    // otherwise leave what's left to the callback as well
    _output_buffer.write(
        data + number_of_bytes_sent, data_size - number_of_bytes_sent
    );

    _pollable_file_descriptor.enable_write_event();
    _is_writing = true;
}

int TcpConnectSocketfd::_get_socket_errno(int socketfd) {
    int saved_errno;
    socklen_t saved_errno_len = sizeof(decltype(saved_errno));

    if (::getsockopt(
            socketfd, SOL_SOCKET, SO_ERROR, &saved_errno, &saved_errno_len
        )
        < 0) {
        LOG_SYS_FATAL << "getsockopt failed";
    }
    else {
        return saved_errno;
    }
}

void TcpConnectSocketfd::_read_event_callback() {
    _receive_all_data();

    // user should call `send()` internally to send the data and possibly start
    // listening the writing event
    _message_callback(this, &_input_buffer, util::TimePoint());
}

void TcpConnectSocketfd::_write_event_callback() {
    auto total_number_of_bytes = _output_buffer.get_readable_size();

    auto number_of_bytes_sent = _send_as_many_data(
        _output_buffer.get_current_read_position(), total_number_of_bytes
    );

    _output_buffer.forward_read_position(number_of_bytes_sent);

    // TCP buffer is full, leave what's left till the next time
    if (number_of_bytes_sent < total_number_of_bytes) {
        return;
    }

    if (_write_complete_callback) {
        _write_complete_callback(this);
    }

    _pollable_file_descriptor.disable_write_event();
    _is_writing = false;

    // do what close event handler should do as all events are no longer being
    // listened on
    if (!_is_reading) {
        if (_close_callback) {
            _close_callback(this);
        }
    }
}

void TcpConnectSocketfd::_close_event_callback() {
    _pollable_file_descriptor.disable_all_event();
    _is_reading = false;
    _is_writing = false;

    if (_close_callback) {
        _close_callback(this);
    }
}

void TcpConnectSocketfd::_error_event_callback() {
    auto saved_errno = _get_socket_errno(_pollable_file_descriptor.get_fd());

    LOG_ERROR << "TCP connection socket error, id: " << _id
              << ", errno: " << saved_errno
              << ", description: " << util::strerror_tl(saved_errno);
}

void TcpConnectSocketfd::_receive_all_data() {
    while (true) {
        _input_buffer.make_space(_RECEIVE_DATA_SIZE);

        ssize_t bytes_read = ::recv(
            _pollable_file_descriptor.get_fd(),
            _input_buffer.get_current_read_position(),
            _RECEIVE_DATA_SIZE,
            0
        );

        // have read in some data but maybe not all
        if (bytes_read > 0) {
            continue;
        }

        // EOF, i.e. the peer have closed its write end if not both ends of
        // connection
        else if (bytes_read == 0) {
            _pollable_file_descriptor.disable_read_event();
            _is_reading = false;

            // did not close connection here as the peer might still have its
            // read end opened

            break;
        }

        // read falis
        else {
            // current iteration of reading is finished
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            // actual error occured
            else {
                // get system errno
                LOG_SYS_ERROR << "falied when reading from the socket";

                // get socketfd errno
                _error_event_callback();
            }
        }
    }
}

size_t
TcpConnectSocketfd::_send_as_many_data(const char *data, size_t data_size) {
    size_t total_number_of_bytes_sent = 0;

    while (total_number_of_bytes_sent < data_size) {
        ssize_t current_number_of_bytes_sent = ::send(
            _pollable_file_descriptor.get_fd(),
            data + total_number_of_bytes_sent,
            data_size - total_number_of_bytes_sent,
            0
        );

        if (current_number_of_bytes_sent >= 0) {
            total_number_of_bytes_sent +=
                static_cast<size_t>(current_number_of_bytes_sent);
        }

        else if (current_number_of_bytes_sent == -1) {
            // socket's send buffer is full
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            // actual error occured
            else {
                // get system errno
                LOG_SYS_ERROR << "falied when writing to the socket";

                // get socketfd errno
                _error_event_callback();
            }
        }
    }

    return total_number_of_bytes_sent;
}

} // namespace xubinh_server