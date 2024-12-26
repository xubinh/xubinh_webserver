#ifndef __XUBINH_SERVER_POLLABLE_FILE_DESCRIPTOR
#define __XUBINH_SERVER_POLLABLE_FILE_DESCRIPTOR

#include <cstdint>
#include <functional>
#include <memory>
#include <sys/epoll.h>
#include <unistd.h>

#include "util/time_point.h"

namespace xubinh_server {

class EventLoop;

// pollable file descriptors
//
// - not thread-safe
class PollableFileDescriptor {
private:
    using EpollEventsType = decltype(epoll_event{}.events);

public:
    using ReadEventCallbackType =
        std::function<void(util::TimePoint time_stamp)>;

    using WriteEventCallbackType = std::function<void()>;

    using CloseEventCallbackType = std::function<void()>;

    using ErrorEventCallbackType = std::function<void()>;

    static void set_fd_as_blocking(int fd);

    static void set_fd_as_nonblocking(int fd);

    PollableFileDescriptor(
        int fd,
        EventLoop *event_loop,
        bool prefer_et = true,
        bool need_set_non_blocking = true
    );

    // no copy
    PollableFileDescriptor(const PollableFileDescriptor &) = delete;
    PollableFileDescriptor &operator=(const PollableFileDescriptor &) = delete;

    // no move
    PollableFileDescriptor(PollableFileDescriptor &&) = delete;
    PollableFileDescriptor &operator=(PollableFileDescriptor &&) = delete;

    // should not close the underlying fd, since wrapper class (i.e.
    // PreconnectSocketfd) might want to transfer its ownership
    ~PollableFileDescriptor() = default;

    int get_fd() const {
        return _fd;
    }

    void close_fd() const;

    void register_read_event_callback(
        const ReadEventCallbackType &read_event_callback
    ) {
        _read_event_callback = read_event_callback;
    }

    void
    register_read_event_callback(ReadEventCallbackType &&read_event_callback) {
        _read_event_callback = std::move(read_event_callback);
    }

    void register_write_event_callback(
        const WriteEventCallbackType &write_event_callback
    ) {
        _write_event_callback = write_event_callback;
    }

    void
    register_write_event_callback(WriteEventCallbackType &&write_event_callback
    ) {
        _write_event_callback = std::move(write_event_callback);
    }

    void register_close_event_callback(
        const CloseEventCallbackType &close_event_callback
    ) {
        _close_event_callback = close_event_callback;
    }

    void
    register_close_event_callback(CloseEventCallbackType &&close_event_callback
    ) {
        _close_event_callback = std::move(close_event_callback);
    }

    void register_error_event_callback(
        const ErrorEventCallbackType &error_event_callback
    ) {
        _error_event_callback = error_event_callback;
    }

    void
    register_error_event_callback(ErrorEventCallbackType &&error_event_callback
    ) {
        _error_event_callback = std::move(error_event_callback);
    }

    void enable_read_event();

    void disable_read_event();

    void enable_write_event();

    void disable_write_event();

    void detach_from_poller();

    bool is_reading() const {
        return _is_reading;
    }

    bool is_writing() const {
        return _is_writing;
    }

    bool is_detached() const {
        return _is_detached;
    }

    // used by poller
    void set_active_events(uint32_t active_events) {
        _active_events = active_events;
    }

    // used by poller
    void dispatch_active_events(util::TimePoint time_stamp);

    void register_weak_lifetime_guard(
        const std::shared_ptr<void> &strong_lifetime_guard
    ) {
        _weak_lifetime_guard = strong_lifetime_guard;
        _is_weak_lifetime_guard_registered = true;
    }

    void reset_to(int new_fd);

private:
    void _register_event();

    void _dispatch_active_events(util::TimePoint time_stamp);

    bool _is_reading = false;
    bool _is_writing = false;
    bool _is_detached = true;

    int _fd;
    EventLoop *_event_loop;
    EpollEventsType _initial_epoll_event;
    epoll_event _event{};
    uint32_t _active_events{};
    ReadEventCallbackType _read_event_callback;
    WriteEventCallbackType _write_event_callback;
    CloseEventCallbackType _close_event_callback;
    ErrorEventCallbackType _error_event_callback;
    std::weak_ptr<void> _weak_lifetime_guard;
    bool _is_weak_lifetime_guard_registered = false;
};

} // namespace xubinh_server

#endif