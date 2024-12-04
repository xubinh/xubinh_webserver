#include "timerfd.h"
#include "log_builder.h"

namespace xubinh_server {

// assume
void Timerfd::set_alarm_at_time_point(const TimePoint &time_point) {
    // always one-off, repetition is controlled from outside
    itimerspec timer_specification{};

    time_point.to_timespec(&timer_specification.it_value);

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

    else if (bytes_read < sizeof(number_of_expirations)) {
        LOG_WARN << "incomplete timerfd read operation";

        return 0;
    }

    return number_of_expirations;
}

void Timerfd::cancel() {
    // all zero for timer cancellation
    itimerspec timer_specification{};

    if (timerfd_settime(_fd, 0, &timer_specification, nullptr) == -1) {
        LOG_SYS_FATAL << "timerfd_settime failed";
    }
}

} // namespace xubinh_server