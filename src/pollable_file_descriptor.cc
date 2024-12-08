#include <fcntl.h>
#include <functional>
#include <sys/epoll.h>

#include "event_poller.h"
#include "pollable_file_descriptor.h"

namespace xubinh_server {

void PollableFileDescriptor::set_fd_as_nonblocking(int fd) {
    auto flags = fcntl(fd, F_GETFL, 0);

    if (flags == -1) {
        LOG_SYS_FATAL << "fcntl F_GETFL";
    }

    flags |= O_NONBLOCK;

    if (fcntl(fd, F_SETFL, flags) == -1) {
        LOG_SYS_FATAL << "fcntl F_SETFL";
    }

    return;
}

void PollableFileDescriptor::enable_read_event() {
    _event.events |= EventPoller::EVENT_TYPE_READ;

    _register_event();
}

void PollableFileDescriptor::disable_read_event() {
    _event.events &= ~EventPoller::EVENT_TYPE_READ;

    _register_event();
}

void PollableFileDescriptor::enable_write_event() {
    _event.events |= EventPoller::EVENT_TYPE_WRITE;

    _register_event();
}

void PollableFileDescriptor::disable_write_event() {
    _event.events &= ~EventPoller::EVENT_TYPE_WRITE;

    _register_event();
}

void PollableFileDescriptor::enable_all_event() {
    _event.events |= EventPoller::EVENT_TYPE_ALL;

    _register_event();
}

void PollableFileDescriptor::disable_all_event() {
    _event.events &= ~EventPoller::EVENT_TYPE_ALL;

    _register_event();
}

void PollableFileDescriptor::dispatch_active_events() {
    if (_active_events & EventPoller::EVENT_TYPE_READ) {
        if (_read_event_callback) {
            _read_event_callback();
        }
    }

    if (_active_events & EventPoller::EVENT_TYPE_WRITE) {
        if (_write_event_callback) {
            _write_event_callback();
        }
    }

    // here only means that the client peer closed its end of the channel
    if (_active_events & EventPoller::EVENT_TYPE_CLOSE) {
        if (_close_event_callback) {
            _close_event_callback();
        }
    }

    if (_active_events & EventPoller::EVENT_TYPE_ERROR) {
        if (_error_event_callback) {
            _error_event_callback();
        }
    }
}

void PollableFileDescriptor::_register_event() {
    _event_loop->run(
        std::bind(EventLoop::register_event_for_fd, _event_loop, _fd, &_event)
    );
}

} // namespace xubinh_server