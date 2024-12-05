#include "timerfd.h"
#include "log_builder.h"

namespace xubinh_server {

// - thread-safe
// - always one-off, repetition is controlled from outside
void Timerfd::set_alarm_at_time_point(const TimePoint &time_point) {
    itimerspec timer_specification{};

    time_point.to_timespec(&timer_specification.it_value);

    // thread-safe
    if (timerfd_settime(
            _fd,
            TFD_TIMER_ABSTIME | TFD_TIMER_CANCEL_ON_SET,
            &timer_specification,
            nullptr
        )
        == -1) {
        LOG_SYS_FATAL << "timerfd_settime failed";
    }
}

// thread-safe
uint64_t Timerfd::retrieve_the_number_of_expirations() {
    uint64_t number_of_expirations;

    auto bytes_read =
        ::read(_fd, &number_of_expirations, sizeof(number_of_expirations));

    if (bytes_read == -1) {
        if (errno == ECANCELED) {
            LOG_SYS_ERROR << "timerfd has been canceled due to discontinuous "
                             "changes to the clock";
        }
        else if (errno == EAGAIN) {
            LOG_WARN << "timerfd is read before expiration occurs";
        }
        else {
            LOG_SYS_ERROR << "failed reading from timerfd";
        }

        return 0;
    }

    // [TODO]: delete this branch
    else if (bytes_read < sizeof(number_of_expirations)) {
        LOG_WARN << "incomplete timerfd read operation";

        return 0;
    }

    return number_of_expirations;
}

// thread-safe
void Timerfd::cancel() {
    // all zero for timer cancellation
    itimerspec timer_specification{};

    if (timerfd_settime(_fd, 0, &timer_specification, nullptr) == -1) {
        LOG_SYS_FATAL << "timerfd_settime failed";
    }
}
// Issue a cancellation at the point where a timer is already expired but
// before the other thread polls the event and new timers are rearmed would
// still cancel that already-expired timer, but in this case only the counter
// will be zeroed out (?) and the poll event is still triggered nonetheless
// which would cause the poller to be blocked indefinitely if the underlying fd
// was not set to non-block mode in the first place. The solution is to set
// non-blocking of the fd and the read would fail and return a errno indicating
// that situation.
// [source](https://stackoverflow.com/questions/78381098/is-timerfd-thread-safe)

} // namespace xubinh_server