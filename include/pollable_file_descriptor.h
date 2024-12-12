#ifndef XUBINH_SERVER_POLLABLE_FILE_DESCRIPTOR
#define XUBINH_SERVER_POLLABLE_FILE_DESCRIPTOR

#include <cstdint>
#include <functional>

#include "event_loop.h"

namespace xubinh_server {

// not thread-safe
class PollableFileDescriptor {
public:
    using CallbackType = std::function<void()>;

    static void set_fd_as_nonblocking(int fd);

    PollableFileDescriptor(int fd, EventLoop *event_loop)
        : _fd(fd), _event_loop(event_loop) {

        _event.data.ptr = this;
        _event.events |= EPOLLET;

        set_fd_as_nonblocking(_fd);
    }

    int get_fd() {
        return _fd;
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

    void register_weak_lifetime_guard(
        const std::shared_ptr<void> &strong_lifetime_guard
    ) {
        _weak_lifetime_guard = strong_lifetime_guard;
        _is_weak_lifetime_guard_registered = true;
    }

private:
    void _register_event();

    void _dispatch_active_events();

    int _fd;
    EventLoop *_event_loop;
    epoll_event _event{};
    uint32_t _active_events{};
    CallbackType _read_event_callback;
    CallbackType _write_event_callback;
    CallbackType _close_event_callback;
    CallbackType _error_event_callback;
    std::weak_ptr<void> _weak_lifetime_guard;
    bool _is_weak_lifetime_guard_registered = false;
};

} // namespace xubinh_server

#endif