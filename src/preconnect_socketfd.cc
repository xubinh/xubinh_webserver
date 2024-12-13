#include <sys/socket.h>

#include "preconnect_socketfd.h"

namespace xubinh_server {

PreconnectSocketfd::PreconnectSocketfd(
    int fd,
    EventLoop *event_loop,
    const InetAddress &server_address,
    int max_number_of_retries
)
    : _event_loop(event_loop), _MAX_NUMBER_OF_RETRIES(max_number_of_retries),
      _server_address(server_address),
      _pollable_file_descriptor(fd, event_loop) {

    if (fd < 0) {
        LOG_SYS_FATAL << "invalid file descriptor (must be non-negative)";
    }

    _pollable_file_descriptor.register_write_event_callback(
        std::bind(_write_event_callback, this)
    );

    // must ensure lifetime safety as this exact object could be
    // destroyed by the callbacks registered inside themselves
    _pollable_file_descriptor.register_weak_lifetime_guard(shared_from_this());
}

int PreconnectSocketfd::_connect_to_server(
    int socketfd, const InetAddress &server_address
) {
    return ::connect(
        socketfd,
        server_address.get_address(),
        server_address.get_address_length()
    );
}

void PreconnectSocketfd::_schedule_retry() {
    if (_number_of_retries >= _MAX_NUMBER_OF_RETRIES) {
        LOG_ERROR << "max number of retries reached";

        if (_connect_fail_callback) {
            _connect_fail_callback(this);
        }

        return;
    }

    _number_of_retries += 1;

    _event_loop->run_after_time_interval(
        std::min(_MAX_RETRY_TIME_INTERVAL, _current_retry_time_interval),
        0,
        0,
        std::bind(_try_once, shared_from_this())
    );

    _current_retry_time_interval *= 2;
}

void PreconnectSocketfd::_write_event_callback() {
    int socketfd = _pollable_file_descriptor.get_fd();

    auto saved_errno = get_socketfd_errno(socketfd);

    // success
    if (saved_errno == 0) {
        _new_connection_callback(this, socketfd);

        _pollable_file_descriptor.disable_write_event();
    }

    // must be retryable errors since otherwise would have already been blocked
    // by `_try_once()`
    else {
        _schedule_retry();
    }
}

void PreconnectSocketfd::_try_once() {
    int socketfd = _pollable_file_descriptor.get_fd();

    auto connect_status =
        PreconnectSocketfd::_connect_to_server(socketfd, _server_address);

    // success
    if (connect_status == 0) {
        _new_connection_callback(this, socketfd);

        _pollable_file_descriptor.disable_write_event();
    }

    // error
    else {
        switch (errno) {

        // for non-blocking socketfd: success later, not now
        case EINPROGRESS: {
            _pollable_file_descriptor.enable_write_event();

            break;
        }

        // may retry
        case EADDRINUSE:
        case EADDRNOTAVAIL:
        case EAGAIN:
        case ECONNREFUSED:
        case ENETUNREACH: {
            _schedule_retry();

            break;
        }

        // one socketfd, one object
        case EALREADY:
        case EISCONN: {
            LOG_SYS_ERROR << "there seems to be multiple entities trying to "
                             "connect one single socketfd";

            if (_connect_fail_callback) {
                _connect_fail_callback(this);
            }

            break;
        }

        case EACCES:
        case EPERM:
        case EAFNOSUPPORT:
        case EBADF:
        case EFAULT:
        case ENOTSOCK:
        case EPROTOTYPE: {
            LOG_SYS_ERROR << "invalid connect operation";

            if (_connect_fail_callback) {
                _connect_fail_callback(this);
            }

            break;
        }

        // [NOTE]: signals will be managed by the user using Signalfd, so
        // here just make it abort
        // case EINTR:
        default: {
            LOG_SYS_FATAL << "unknown error";

            break;
        }
        }
    }
}

} // namespace xubinh_server