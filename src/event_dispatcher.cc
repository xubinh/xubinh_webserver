#include <functional>
#include <sys/epoll.h>

#include "event_dispatcher.h"

namespace xubinh_server {

void EventDispatcher::enable_read_event() {
    _event.events |= EPOLLIN;
    // no need for `EPOLLONESHOT` since at most one thread will be responsible
    // for listening on a fd

    _hosted_event_loop->run(std::bind(
        EventLoop::register_event_for_fd,
        _hosted_event_loop,
        _hosted_fd,
        &_event
    ));
}

void EventDispatcher::disable_read_event() {
    _event.events &= ~EPOLLIN;

    _hosted_event_loop->run(std::bind(
        EventLoop::register_event_for_fd,
        _hosted_event_loop,
        _hosted_fd,
        &_event
    ));
}

void EventDispatcher::enable_write_event() {
    _event.events |= EPOLLOUT;
    // no need for `EPOLLONESHOT` since at most one thread will be responsible
    // for listening on a fd

    _hosted_event_loop->run(std::bind(
        EventLoop::register_event_for_fd,
        _hosted_event_loop,
        _hosted_fd,
        &_event
    ));
}

void EventDispatcher::disable_write_event() {
    _event.events &= ~EPOLLOUT;

    _hosted_event_loop->run(std::bind(
        EventLoop::register_event_for_fd,
        _hosted_event_loop,
        _hosted_fd,
        &_event
    ));
}

void EventDispatcher::dispatch_active_events() {
    if (_active_events & (EPOLLIN | EPOLLPRI)) {
        _read_event_callback();
    }

    if (_active_events & EPOLLOUT) {
        _write_event_callback();
    }

    if (_active_events & (EPOLLRDHUP | EPOLLHUP)) {
        _close_event_callback();
    }

    if (_active_events & EPOLLERR) {
        _error_event_callback();
    }
}

} // namespace xubinh_server