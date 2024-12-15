#ifndef __XUBINH_SERVER_TIMERFD
#define __XUBINH_SERVER_TIMERFD

#include <sys/timerfd.h>

#include "log_builder.h"
#include "pollable_file_descriptor.h"
#include "util/time_point.h"

namespace xubinh_server {

class EventLoop;

class Timerfd {
private:
    using TimePoint = util::TimePoint;

public:
    using MessageCallbackType = std::function<void(uint64_t)>;

    static int create_timerfd(int flags) {
        if (flags == 0) {
            flags = _DEFAULT_TIMERFD_FLAGS;
        }

        return ::timerfd_create(CLOCK_REALTIME, _DEFAULT_TIMERFD_FLAGS);
    }

    Timerfd(int fd, EventLoop *event_loop)
        : _pollable_file_descriptor(fd, event_loop) {

        if (fd < 0) {
            LOG_SYS_FATAL << "invalid file descriptor (must be non-negative)";
        }

        _pollable_file_descriptor.register_read_event_callback(
            std::bind(&Timerfd::_read_event_callback, this)
        );
    }

    ~Timerfd() {
        _pollable_file_descriptor.disable_read_event();

        ::close(_pollable_file_descriptor.get_fd());
    }

    void register_timerfd_message_callback(
        MessageCallbackType timerfd_message_callback
    ) {
        _message_callback = std::move(timerfd_message_callback);
    }

    void start() {
        if (!_message_callback) {
            LOG_FATAL << "missing message callback";
        }

        _pollable_file_descriptor.enable_read_event();
    }

    // async write operation
    //
    // - thread-safe
    // - always one-off, repetition is controlled from outside
    void set_alarm_at_time_point(const TimePoint &time_point);

    // write operation
    //
    // - thread-safe
    void cancel();
    // Issue a cancellation at the point where a timer is already expired but
    // before the other thread polls the event and new timers are rearmed would
    // still cancel that already-expired timer, but in this case only the
    // counter will be zeroed out (?) and the poll event is still triggered
    // nonetheless which would cause the poller to be blocked indefinitely if
    // the underlying fd was not set to non-block mode in the first place. The
    // solution is to set non-blocking of the fd and the read would fail and
    // return a errno indicating that situation.
    // [source](https://stackoverflow.com/questions/78381098/is-timerfd-thread-safe)

private:
    // must be zero before (and including) Linux v2.6.26
    static constexpr int _DEFAULT_TIMERFD_FLAGS = TFD_CLOEXEC | TFD_NONBLOCK;

    void _read_event_callback() {
        _message_callback(_retrieve_the_number_of_expirations());
    }

    // read operation
    //
    // - thread-safe
    uint64_t _retrieve_the_number_of_expirations();

    MessageCallbackType _message_callback;

    PollableFileDescriptor _pollable_file_descriptor;
};

} // namespace xubinh_server

#endif