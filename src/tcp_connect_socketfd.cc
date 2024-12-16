#include <sys/socket.h>
#include <sys/types.h>

#include "event_loop.h"
#include "log_builder.h"
#include "tcp_connect_socketfd.h"
#include "util/errno.h"
#include "util/time_point.h"

namespace xubinh_server {

TcpConnectSocketfd::TcpConnectSocketfd(
    int fd,
    EventLoop *event_loop,
    const std::string &id,
    const InetAddress &local_address,
    const InetAddress &remote_address
)
    : _id(id), _local_address(local_address), _remote_address(remote_address),
      _pollable_file_descriptor(fd, event_loop) {

    if (fd < 0) {
        LOG_SYS_FATAL << "invalid file descriptor (must be non-negative)";
    }
}

void TcpConnectSocketfd::start() {
    if (_is_reading) {
        LOG_ERROR << "try to start an already started tcp connect socketfd";

        return;
    }

    if (!_message_callback) {
        LOG_FATAL << "missing message callback";
    }

    _pollable_file_descriptor.register_read_event_callback(
        std::bind(&TcpConnectSocketfd::_read_event_callback, shared_from_this())
    );

    _pollable_file_descriptor.register_write_event_callback(std::bind(
        &TcpConnectSocketfd::_write_event_callback, shared_from_this()
    ));

    _pollable_file_descriptor.register_close_event_callback(std::bind(
        &TcpConnectSocketfd::_close_event_callback, shared_from_this()
    ));

    _pollable_file_descriptor.register_error_event_callback(std::bind(
        &TcpConnectSocketfd::_error_event_callback, shared_from_this()
    ));

    // must ensure lifetime safety as this exact object could be
    // destroyed by the callbacks registered inside themselves
    _pollable_file_descriptor.register_weak_lifetime_guard(shared_from_this());

    _is_reading = true; // must set before the enabling to ensure thread-safety
    _pollable_file_descriptor.enable_read_event();
}

void TcpConnectSocketfd::shutdown() {
    // must be called in the loop to ensure thread-safety of internal state
    _pollable_file_descriptor.get_loop()->run(std::bind(
        &TcpConnectSocketfd::_close_event_callback, shared_from_this()
    ));
}

void TcpConnectSocketfd::send(const char *data, size_t data_size) {
    // could be called after the connection is closed, so check it first
    if (_is_closed) {
        return;
    }

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
            _write_complete_callback(shared_from_this());
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

void TcpConnectSocketfd::_read_event_callback() {
    LOG_DEBUG << "tcp connect socketfd read event encountered";

    // fd -- W --> input buffer
    _receive_all_data();

    // user should call `send()` internally to send the data and possibly start
    // listening the writing event
    //
    // input buffer <-- R -- user -- W --> output buffer
    _message_callback(shared_from_this(), &_input_buffer, util::TimePoint());

    // close connection if (1) the peer has closed its write end, (2) all data
    // is read and processed, and (3) no data needs to be sent to the peer
    if (!_is_reading && !_output_buffer.get_readable_size()) {
        _close_event_callback();
    }
}

void TcpConnectSocketfd::_write_event_callback() {
    LOG_DEBUG << "tcp connect socketfd write event encountered";

    // could be called after the connection is closed, so check it first
    if (_is_closed) {
        return;
    }

    auto total_number_of_bytes = _output_buffer.get_readable_size();

    // output buffer -- W --> fd
    auto number_of_bytes_sent = _send_as_many_data(
        _output_buffer.get_current_read_position(), total_number_of_bytes
    );

    _output_buffer.forward_read_position(number_of_bytes_sent);

    // TCP buffer is full, leave what's left till the next time
    if (number_of_bytes_sent < total_number_of_bytes) {
        return;
    }

    if (_write_complete_callback) {
        _write_complete_callback(shared_from_this());
    }

    _pollable_file_descriptor.disable_write_event();
    _is_writing = false;

    // close connection if (1) the peer has closed its write end, (2) all data
    // is read and processed, and (3) no data needs to be sent to the peer
    if (!_is_reading) {
        _close_event_callback();
    }
}

void TcpConnectSocketfd::_close_event_callback() {
    LOG_DEBUG << "tcp connect socketfd close event encountered";

    if (_is_closed) {
        return;
    }

    // no more reads or writes will come in ET mode; OK to disconnect it from
    // the loop
    _pollable_file_descriptor.detach_from_poller();
    _is_closed = true;

    // [NOTE]: still needs to read in data, which can be done within this
    // iteration

    if (_close_callback) {
        _close_callback(shared_from_this());
    }
}

void TcpConnectSocketfd::_error_event_callback() {
    LOG_DEBUG << "tcp connect socketfd error event encountered";

    auto saved_errno = get_socketfd_errno(_pollable_file_descriptor.get_fd());

    LOG_ERROR << "TCP connection socket error, id: " << _id
              << ", errno: " << saved_errno
              << ", description: " << util::strerror_tl(saved_errno);
}

void TcpConnectSocketfd::_receive_all_data() {
    while (true) {
        _input_buffer.make_space(_RECEIVE_DATA_SIZE);

        ssize_t bytes_read = ::recv(
            _pollable_file_descriptor.get_fd(),
            _input_buffer.get_current_write_position(),
            _RECEIVE_DATA_SIZE,
            0
        );

        // have read in some data but maybe not all
        if (bytes_read > 0) {
            _input_buffer.forward_write_position(bytes_read);

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
    if (data_size == 0) {
        return 0;
    }

    size_t total_number_of_bytes_sent = 0;

    while (total_number_of_bytes_sent < data_size) {
        ssize_t current_number_of_bytes_sent = ::send(
            _pollable_file_descriptor.get_fd(),
            data + total_number_of_bytes_sent,
            data_size - total_number_of_bytes_sent,
            MSG_NOSIGNAL // prevent shutting down by a single SIGPIPE
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

            // tries to write after the peer has closed the connection
            else if (errno == EPIPE) {
                LOG_WARN << "SIGPIPE encountered when trying to write to a tcp "
                            "connection";

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