#ifndef __XUBINH_SERVER_POLLABLE_FILE_DESCRIPTOR
#define __XUBINH_SERVER_POLLABLE_FILE_DESCRIPTOR

#include <cstdint>
#include <functional>
#include <memory>
#include <sys/epoll.h>
#include <unistd.h>

namespace xubinh_server {

class EventLoop;

// non-copyable, non-movable, and pollable file descriptors
//
// - not thread-safe
class PollableFileDescriptor {
private:
    using EpollEventsType = decltype(epoll_event{}.events);

public:
    using CallbackType = std::function<void()>;

    static void set_fd_as_nonblocking(int fd);

    PollableFileDescriptor(int fd, EventLoop *event_loop)
        : _fd(fd), _event_loop(event_loop) {

        _event.data.ptr = this;
        _event.events = _INITIAL_EPOLL_EVENT;

        set_fd_as_nonblocking(_fd);
    }

    // no copy
    PollableFileDescriptor(const PollableFileDescriptor &) = delete;
    PollableFileDescriptor &operator=(const PollableFileDescriptor &) = delete;

    // no move
    PollableFileDescriptor(PollableFileDescriptor &&) = delete;
    PollableFileDescriptor &operator=(PollableFileDescriptor &&) = delete;

    // should not close the underlying fd, since wrapper class (i.e.
    // PreconnectSocketfd) might want to transfer its ownership
    ~PollableFileDescriptor() = default;

    int get_fd() {
        return _fd;
    }

    EventLoop *get_loop() {
        return _event_loop;
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

    void detach_from_poller();

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
    void _register_event(EpollEventsType new_events);

    void _dispatch_active_events();

    static constexpr EpollEventsType _INITIAL_EPOLL_EVENT = EPOLLET;

    bool _is_attached = false;

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