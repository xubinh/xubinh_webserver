#include <sys/socket.h>

#include "event_loop.h"
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
        std::bind(&PreconnectSocketfd::_write_event_callback, this)
    );
}

void PreconnectSocketfd::start() {
    if (!_new_connection_callback) {
        LOG_FATAL << "missing new connection callback";
    }

    // must ensure lifetime safety as this exact object could be
    // destroyed by the callbacks registered inside themselves
    _pollable_file_descriptor.register_weak_lifetime_guard(shared_from_this());

    _event_loop->run(
        std::bind(&PreconnectSocketfd::_try_once, shared_from_this())
    );
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
            _connect_fail_callback();
        }

        return;
    }

    _number_of_retries += 1;

    _event_loop->run_after_time_interval(
        std::min(
            PreconnectSocketfd::_MAX_RETRY_TIME_INTERVAL,
            _current_retry_time_interval
        ),
        0,
        0,
        std::bind(&PreconnectSocketfd::_try_once, shared_from_this())
    );

    _current_retry_time_interval *= 2;
}

void PreconnectSocketfd::_write_event_callback() {
    int socketfd = _pollable_file_descriptor.get_fd();

    auto saved_errno = get_socketfd_errno(socketfd);

    // success
    if (saved_errno == 0) {
        _pollable_file_descriptor.disable_write_event();

        _new_connection_callback(socketfd);
    }

    // must be retryable errors since otherwise would have already been blocked
    // by `_try_once()`
    else {
        _schedule_retry();
    }
}

void PreconnectSocketfd::_try_once() {
    LOG_DEBUG << "try connecting to " + _server_address.to_string() + " ...";

    int socketfd = _pollable_file_descriptor.get_fd();

    auto connect_status =
        PreconnectSocketfd::_connect_to_server(socketfd, _server_address);

    // success
    if (connect_status == 0) {
        _new_connection_callback(socketfd);

        _pollable_file_descriptor.disable_write_event();
    }

    // error
    else {
        switch (errno) {
        // for non-blocking socketfd: success later, not now
        case EINPROGRESS:
            _pollable_file_descriptor.enable_write_event();

            break;

        // may retry
        case EADDRINUSE:
        case EADDRNOTAVAIL:
        case EAGAIN:
        case ECONNREFUSED:
        case ENETUNREACH:
            _schedule_retry();

            break;

        // one socketfd, one object
        case EALREADY:
        case EISCONN:
            LOG_SYS_ERROR << "there seems to be multiple entities trying to "
                             "connect one single socketfd";

            if (_connect_fail_callback) {
                _connect_fail_callback();
            }

            break;

        case EACCES:
        case EPERM:
        case EAFNOSUPPORT:
        case EBADF:
        case EFAULT:
        case ENOTSOCK:
        case EPROTOTYPE:
            LOG_SYS_ERROR << "invalid connect operation";

            if (_connect_fail_callback) {
                _connect_fail_callback();
            }

            break;

        // [NOTE]: signals will be managed by the user using Signalfd, so
        // here just make it abort
        // case EINTR:
        default:
            LOG_SYS_FATAL << "unknown error";

            break;
        }
    }
}

const util::TimeInterval PreconnectSocketfd::_INITIAL_RETRY_TIME_INTERVAL{
    static_cast<int64_t>(500 * 1000 * 1000)}; // 0.5 sec

const util::TimeInterval PreconnectSocketfd::_MAX_RETRY_TIME_INTERVAL{
    static_cast<int64_t>(30 * 1000 * 1000)
    * static_cast<int64_t>(1000)}; // 30 sec

} // namespace xubinh_server