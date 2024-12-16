#include <fcntl.h>
#include <functional>
#include <sys/epoll.h>

#include "event_loop.h"
#include "event_poller.h"
#include "log_builder.h"
#include "pollable_file_descriptor.h"

namespace xubinh_server {

void PollableFileDescriptor::set_fd_as_nonblocking(int fd) {
    auto flags = ::fcntl(fd, F_GETFL, 0);

    if (flags == -1) {
        LOG_SYS_FATAL << "fcntl F_GETFL";
    }

    flags |= O_NONBLOCK;

    if (::fcntl(fd, F_SETFL, flags) == -1) {
        LOG_SYS_FATAL << "fcntl F_SETFL";
    }

    return;
}

void PollableFileDescriptor::enable_read_event() {
    _register_event(_event.events | EventPoller::EVENT_TYPE_READ);
}

void PollableFileDescriptor::disable_read_event() {
    _register_event(_event.events & ~EventPoller::EVENT_TYPE_READ);
}

void PollableFileDescriptor::enable_write_event() {
    _register_event(_event.events | EventPoller::EVENT_TYPE_WRITE);
}

void PollableFileDescriptor::disable_write_event() {
    _register_event(_event.events & ~EventPoller::EVENT_TYPE_WRITE);
}

void PollableFileDescriptor::detach_from_poller() {
    if (!_is_attached) {
        return;
    }

    _event_loop->run(
        std::bind(&EventLoop::detach_fd_from_poller, _event_loop, _fd)
    );

    _event.events = _INITIAL_EPOLL_EVENT; // reset

    _is_attached = false;
}

void PollableFileDescriptor::dispatch_active_events() {
    // if lifetime guard is registed, dispatch events only after locking up the
    // guard
    if (_is_weak_lifetime_guard_registered) {
        auto strong_lifetime_guard = _weak_lifetime_guard.lock();

        if (strong_lifetime_guard) {
            _dispatch_active_events();
        }

        else {
            // the wrapper object is just in the process of destruction right
            // now
            return;
        }
    }

    // otherwise the outer class ensures there will be no lifetime issue
    else {
        _dispatch_active_events();
    }
}

void PollableFileDescriptor::_register_event(EpollEventsType new_events) {
    if (new_events == _event.events) {
        return;
    }

    _event.events = new_events;

    _event_loop->run(
        std::bind(&EventLoop::register_event_for_fd, _event_loop, _fd, &_event)
    );

    _is_attached = true;
}

void PollableFileDescriptor::_dispatch_active_events() {
    // close event first for users set their flags
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
            _read_event_callback();
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