#ifndef XUBINH_SERVER_EVENT_DISPATCHER
#define XUBINH_SERVER_EVENT_DISPATCHER

#include <cstdint>
#include <functional>

#include "event_file_descriptor.h"
#include "event_loop.h"

namespace xubinh_server {

// not thread-safe
class EventDispatcher {
public:
    using CallbackType = std::function<void()>;

    EventDispatcher(int hosted_fd, EventLoop *hosted_event_loop)
        : _hosted_fd(hosted_fd), _hosted_event_loop(hosted_event_loop) {

        _event.data.ptr = this;
        _event.events |= EPOLLET;

        EventFileDescriptor::set_fd_as_nonblocking(_hosted_fd);
    }

    void register_read_event_callback(CallbackType read_event_callback) {
        _read_event_callback = std::move(read_event_callback);
    }

    void register_write_event_callback(CallbackType write_event_callback) {
        _write_event_callback = std::move(write_event_callback);
    }

    void register_close_event_callback(CallbackType close_event_callback) {
        _close_event_callback = std::move(close_event_callback);
    }

    void register_error_event_callback(CallbackType error_event_callback) {
        _error_event_callback = std::move(error_event_callback);
    }

    void enable_read_event();

    void disable_read_event();

    void enable_write_event();

    void disable_write_event();

    void enable_all_event();

    void disable_all_event();

    // used by poller
    void set_active_events(uint32_t active_events) {
        _active_events = active_events;
    }

    // used by poller
    void dispatch_active_events();

private:
    void _register_event();

    // by abstraction, an event dispatcher should not have a fd inside itself,
    // and this is just for simplifying the API
    int _hosted_fd;

    EventLoop *_hosted_event_loop;
    epoll_event _event{};
    uint32_t _active_events{};
    CallbackType _read_event_callback;
    CallbackType _write_event_callback;
    CallbackType _close_event_callback;
    CallbackType _error_event_callback;
};

} // namespace xubinh_server

#endif