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
    EventLoop *loop,
    const uint64_t &id,
    const InetAddress &local_address,
    const InetAddress &remote_address
)
    : _id(id), _local_address(local_address), _remote_address(remote_address),
      _loop(loop),
      _pollable_file_descriptor(
          fd, loop, true, false
      ) /* non-blocking or not is always decided by the outside */ {

    // [NOTE]: this line is for testing
    // disable_socketfd_nagle_algorithm(fd);
}

void TcpConnectSocketfd::start() {
    if (_is_reading()) {
        LOG_ERROR << "try to start an already started tcp connect socketfd";

        return;
    }

    if (!_message_callback) {
        LOG_FATAL << "missing message callback";
    }

    _pollable_file_descriptor.register_read_event_callback(
        [this](util::TimePoint time_stamp) {
            _read_event_callback(time_stamp);
        }
    );

    _pollable_file_descriptor.register_write_event_callback([this]() {
        _write_event_callback();
    });

    _pollable_file_descriptor.register_close_event_callback([this]() {
        _close_event_callback();
    });

    _pollable_file_descriptor.register_error_event_callback([this]() {
        _error_event_callback();
    });

    // must ensure lifetime safety as this exact object could be
    // destroyed by the callbacks registered inside themselves
    _pollable_file_descriptor.register_weak_lifetime_guard(shared_from_this());

    _pollable_file_descriptor.enable_read_event();
}

void TcpConnectSocketfd::shutdown_write() {
    // could be called multiple times, e.g. when the client decided to close the
    // connection immediately after he sent out a HTTP request with a
    // "Connection: close" header in it, which will also issue a
    // shutdown_write operation
    if (_is_write_end_shutdown) {
        return;
    }

    _is_write_end_shutdown = true;

    if (::shutdown(_pollable_file_descriptor.get_fd(), SHUT_WR) == -1) {
        switch (errno) {
        case ENOTCONN:
            LOG_TRACE << "ENOTCONN encountered, connection abort, id: " << _id;

            // the peer does not care what we send to him, so we won't care
            // what he sends to us either
            abort();

            return;

        default:
            LOG_SYS_ERROR
                << "unknown error when shutting down write end of a TCP "
                   "connection, id: "
                << _id;

            _close_event_callback();

            break;
        }
    }

    _pollable_file_descriptor.disable_write_event();

    if (_write_complete_callback) {
        _write_complete_callback(shared_from_this());
    }

    // empty the output buffer, and after which it is the caller's
    // responsibility to keep it that way
    _output_buffer.forward_read_position(_output_buffer.get_readable_size());
}

void TcpConnectSocketfd::abort() {
    if (_is_abotrted) {
        return;
    }

    _close_event_callback();

    auto fd = _pollable_file_descriptor.get_fd();

    struct linger linger_opt;

    linger_opt.l_onoff = 1;  // enable linger
    linger_opt.l_linger = 0; // timeout of 0 seconds (immediate reset)

    // set SO_LINGER with a timeout of 0 to force a TCP RST
    if (::setsockopt(fd, SOL_SOCKET, SO_LINGER, &linger_opt, sizeof(linger_opt))
        == -1) {
        LOG_FATAL << "setsockopt failed";
    }

    // close the socket to trigger the RST
    if (::close(fd) == -1) {
        LOG_FATAL << "close failed";
    }

    _is_write_end_shutdown = true;
    _is_abotrted = true;
}

void TcpConnectSocketfd::check_and_abort(PredicateType predicate) {
    LOG_TRACE << "register event -> worker: _check_and_abort_impl";

    // must use std::bind since move capture lambda is not supported in C++11
    _loop->run(std::bind(
        &TcpConnectSocketfd::_check_and_abort_impl,
        shared_from_this(),
        std::move(predicate)
    ));
}

void TcpConnectSocketfd::send(const char *data, size_t data_size) {
    // could be called after the connection is closed, so check it first
    if (_is_closed()) {
        return;
    }

    // leave the writing to the event callback if already started listening
    if (_is_writing()) {
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
}

uint64_t TcpConnectSocketfd::get_loop_index() const noexcept {
    return _loop->get_loop_index();
}

void TcpConnectSocketfd::_read_event_callback(util::TimePoint time_stamp) {
    LOG_TRACE << "tcp connect socketfd read event encountered, id: " << _id;

    // fd -- W --> input buffer
    _receive_all_data();

    // user should call `send()` internally to send the data and possibly start
    // listening the writing event
    //
    // input buffer <-- R -- user -- W --> output buffer
    _message_callback(shared_from_this(), &_input_buffer, time_stamp);

    // shutdown write if (1) all data is read and processed, (2) the peer closed
    // its write end first, and (3) no data needs to be sent to the peer
    if (!_is_reading() && !_is_write_end_shutdown && !_is_writing()) {
        shutdown_write();
    }
}

void TcpConnectSocketfd::_write_event_callback() {
    LOG_TRACE << "tcp connect socketfd write event encountered, id: " << _id;

    // if write event is triggered at this poll, it means the local did not
    // close its write end when starting this poll, and the close event will be
    // triggered at least till the next poll; but if the close event is also
    // triggered at this poll, then it suggests that there might be something
    // wrong with the peer and the local should stop sending data
    if (_is_closed()) {
        return;
    }

    // the write end might just shutdown in the previous read event
    if (_is_write_end_shutdown) {
        return;
    }

    auto total_number_of_bytes = _output_buffer.get_readable_size();

    // might be zero if the outside wants to deal with the writing by himself,
    // e.g. using zero-copy rather than copying the data back and forth
    if (total_number_of_bytes > 0) {
        // output buffer -- W --> fd
        auto number_of_bytes_sent = _send_as_many_data(
            _output_buffer.get_current_read_position(), total_number_of_bytes
        );

        _output_buffer.forward_read_position(number_of_bytes_sent);

        // TCP buffer is full, leave what's left till the next time
        if (number_of_bytes_sent < total_number_of_bytes) {
            return;
        }
    }

    // might be re-enabled by the outside, so disable it first for it to be
    // overrided
    _pollable_file_descriptor.disable_write_event();

    if (_write_complete_callback) {
        _write_complete_callback(shared_from_this());
    }

    // shutdown write if (1) all data is read and processed, (2) the peer closed
    // its write end first, and (3) no data needs to be sent to the peer
    if (!_is_reading() && !_is_writing()) {
        shutdown_write();
    }
}

void TcpConnectSocketfd::_close_event_callback() {
    LOG_TRACE << "tcp connect socketfd close event encountered, id: " << _id;

    // if (_is_closed()) {
    //     LOG_FATAL << "close event encountered after being closed";
    // }

    // no more reads or writes will come in ET mode; OK to disconnect it from
    // the loop
    _pollable_file_descriptor.detach_from_poller();

    // [NOTE]: the local still needs to read in the data inside the local buffer
    // even though the peer has closed its write end, which can be done within
    // this iteration

    if (_close_callback) {
        _close_callback(shared_from_this());
    }
}

void TcpConnectSocketfd::_error_event_callback() {
    LOG_TRACE << "tcp connect socketfd error event encountered, id: " << _id;

    auto saved_errno = get_socketfd_errno(_pollable_file_descriptor.get_fd());

    LOG_SYS_ERROR << "TCP connection socket error, id: " << _id;
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

        LOG_TRACE << "bytes_read: " << bytes_read;

        // have read in some data but maybe not all
        if (bytes_read > 0) {
            _input_buffer.forward_write_position(bytes_read);

            continue;
        }

        // the peer have closed its write end
        else if (bytes_read == 0) {
            LOG_TRACE << "disable read event";

            _pollable_file_descriptor.disable_read_event();

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

            // the peer abruptly closed its read end (or the whole connection)
            else if (errno == EPIPE) {
                LOG_TRACE << "EPIPE encountered, connection abort, id: " << _id;

                // the peer does not care what we send to him, so we won't care
                // what he sends to us either
                abort();

                return data_size; // fake
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