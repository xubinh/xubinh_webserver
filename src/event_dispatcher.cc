#include <functional>
#include <sys/epoll.h>

#include "event_dispatcher.h"
#include "event_poller.h"

namespace xubinh_server {

void EventDispatcher::enable_read_event() {
    _event.events |= EventPoller::EVENT_TYPE_READ;

    _register_event();
}

void EventDispatcher::disable_read_event() {
    _event.events &= ~EventPoller::EVENT_TYPE_READ;

    _register_event();
}

void EventDispatcher::enable_write_event() {
    _event.events |= EventPoller::EVENT_TYPE_WRITE;

    _register_event();
}

void EventDispatcher::disable_write_event() {
    _event.events &= ~EventPoller::EVENT_TYPE_WRITE;

    _register_event();
}

void EventDispatcher::enable_all_event() {
    _event.events |= EventPoller::EVENT_TYPE_ALL;

    _register_event();
}

void EventDispatcher::disable_all_event() {
    _event.events &= ~EventPoller::EVENT_TYPE_ALL;

    _register_event();
}

void EventDispatcher::dispatch_active_events() {
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

void EventDispatcher::_register_event() {
    _hosted_event_loop->run(std::bind(
        EventLoop::register_event_for_fd,
        _hosted_event_loop,
        _hosted_fd,
        &_event
    ));
}

} // namespace xubinh_server