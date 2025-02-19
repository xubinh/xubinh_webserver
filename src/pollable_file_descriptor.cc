#include <fcntl.h>
#include <functional>
#include <sys/epoll.h>

#include "event_loop.h"
#include "event_poller.h"
#include "log_builder.h"
#include "pollable_file_descriptor.h"

namespace xubinh_server {

void PollableFileDescriptor::set_fd_as_blocking(int fd) {
    auto flags = ::fcntl(fd, F_GETFL, 0);

    if (flags == -1) {
        LOG_SYS_FATAL << "failed to fcntl with F_GETFL flag";
    }

    flags &= ~O_NONBLOCK;

    if (::fcntl(fd, F_SETFL, flags) == -1) {
        LOG_SYS_FATAL << "fcntl F_SETFL";
    }

    return;
}

void PollableFileDescriptor::set_fd_as_nonblocking(int fd) {
    auto flags = ::fcntl(fd, F_GETFL, 0);

    if (flags == -1) {
        LOG_SYS_FATAL << "failed to fcntl with F_GETFL flag";
    }

    flags |= O_NONBLOCK;

    if (::fcntl(fd, F_SETFL, flags) == -1) {
        LOG_SYS_FATAL << "fcntl F_SETFL";
    }

    return;
}

PollableFileDescriptor::PollableFileDescriptor(
    int fd, EventLoop *event_loop, bool prefer_et, bool need_set_non_blocking
)
    : _fd(fd)
    , _event_loop(event_loop)
    , _initial_epoll_event(
          prefer_et ? EPOLLET : static_cast<decltype(EPOLLET)>(0)
      ) {

    if (_fd < 0) {
        LOG_SYS_FATAL << "invalid file descriptor (must be non-negative)";
    }

    _event.data.ptr = this;
    _event.events = _initial_epoll_event;

    if (need_set_non_blocking) {
        set_fd_as_nonblocking(_fd);
    }
}

void PollableFileDescriptor::close_fd() const {
    if (::close(_fd) == -1) {
        LOG_SYS_FATAL << "failed to close fd";
    }
}

void PollableFileDescriptor::enable_read_event() {
    if (_is_reading) {
        return;
    }

    _is_reading = true;

    _event.events |= EventPoller::EVENT_TYPE_READ;

    _register_event();
}

void PollableFileDescriptor::disable_read_event() {
    if (!_is_reading) {
        return;
    }

    _is_reading = false;

    _event.events &= ~EventPoller::EVENT_TYPE_READ;

    _register_event();
}

void PollableFileDescriptor::enable_write_event() {
    if (_is_writing) {
        return;
    }

    _is_writing = true;

    _event.events |= EventPoller::EVENT_TYPE_WRITE;

    _register_event();
}

void PollableFileDescriptor::disable_write_event() {
    if (!_is_writing) {
        return;
    }

    _is_writing = false;

    _event.events &= ~EventPoller::EVENT_TYPE_WRITE;

    _register_event();
}

void PollableFileDescriptor::detach_from_poller() {
    if (_is_detached) {
        return;
    }

    _is_detached = true;

    _is_reading = false;
    _is_writing = false;

    _event.events = _initial_epoll_event;

    LOG_TRACE << "register event -> worker: detach_fd_from_poller";

    _event_loop->run([this]() {
        _event_loop->detach_fd_from_poller(_fd);
    });
}

void PollableFileDescriptor::dispatch_active_events(util::TimePoint time_stamp
) {
    // if lifetime guard is registered, dispatch events only after locking up
    // the guard to prevent self-destruction
    if (_is_weak_lifetime_guard_registered) {
        auto strong_lifetime_guard = _weak_lifetime_guard.lock();

        if (strong_lifetime_guard) {
            _dispatch_active_events(time_stamp);
        }

        else {
            LOG_FATAL << "assert: execution flow never reaches here";
        }
    }

    // otherwise the outer class ensures there will be no lifetime issue
    else {
        _dispatch_active_events(time_stamp);
    }
}

void PollableFileDescriptor::reset_to(int new_fd) {
    if (new_fd < 0) {
        LOG_SYS_FATAL << "invalid file descriptor (must be non-negative)";
    }

    detach_from_poller();

    close_fd();

    _fd = new_fd;
}

void PollableFileDescriptor::_register_event() {
    _is_detached = false;

    LOG_TRACE << "register event -> worker: register_event_for_fd";

    _event_loop->run([this]() {
        _event_loop->register_event_for_fd(_fd, &_event);
    });
}

void PollableFileDescriptor::_dispatch_active_events(util::TimePoint time_stamp
) {
    // deal with close event first, for users to set their flags
    if (_active_events & EventPoller::EVENT_TYPE_CLOSE) {
        if (_close_event_callback) {
            _close_event_callback();
        }
    }

    // then deals with errors
    if (_active_events & EventPoller::EVENT_TYPE_ERROR) {
        if (_error_event_callback) {
            _error_event_callback();
        }
    }

    // with errors cleared, reads in the data, processes it, and possibly writes
    // it out in one go
    if (_active_events & EventPoller::EVENT_TYPE_READ) {
        if (_read_event_callback) {
            _read_event_callback(time_stamp);
        }
    }

    // writes out whatever is left
    if (_active_events & EventPoller::EVENT_TYPE_WRITE) {
        if (_write_event_callback) {
            _write_event_callback();
        }
    }
}

} // namespace xubinh_server