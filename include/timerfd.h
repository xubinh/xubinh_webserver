#ifndef XUBINH_SERVER_TIMERFD
#define XUBINH_SERVER_TIMERFD

#include <sys/timerfd.h>

#include "event_file_descriptor.h"
#include "util/time_point.h"

namespace xubinh_server {

class EventLoop;

class Timerfd : public PollableFileDescriptor {
private:
    using TimePoint = util::TimePoint;

public:
    Timerfd(EventLoop *event_loop)
        : PollableFileDescriptor(
            timerfd_create(CLOCK_REALTIME, _TIMERFD_FLAGS), event_loop
        ) {

        if (_fd == -1) {
            LOG_SYS_FATAL << "failed creating timerfd";
        }
    }

    ~Timerfd() {
        ::close(_fd);
    }

    // async write operation
    //
    // - thread-safe
    // - always one-off, repetition is controlled from outside
    void set_alarm_at_time_point(const TimePoint &time_point);

    // read operation
    //
    // - thread-safe
    uint64_t retrieve_the_number_of_expirations();

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
    static constexpr int _TIMERFD_FLAGS = TFD_CLOEXEC | TFD_NONBLOCK;
};

} // namespace xubinh_server

#endif