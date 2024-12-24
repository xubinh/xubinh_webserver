#include "timerfd.h"
#include "log_builder.h"

namespace xubinh_server {

Timerfd::~Timerfd() {
    _pollable_file_descriptor.detach_from_poller();

    _pollable_file_descriptor.close_fd();

    LOG_INFO << "exit destructor: Timerfd";
}

void Timerfd::start() {
    if (!_message_callback) {
        LOG_FATAL << "missing message callback";
    }

    _pollable_file_descriptor.register_read_event_callback(
        [this](util::TimePoint time_stamp) {
            _read_event_callback(time_stamp);
        }
    );

    _pollable_file_descriptor.enable_read_event();
}

void Timerfd::set_alarm_at_time_point(TimePoint time_point) {
    LOG_DEBUG << "set alarm at time point: "
                     + time_point.to_datetime_string(
                         TimePoint::Purpose::PRINTING
                     );

    itimerspec timer_specification{};

    // only initial expiration, no repetition
    time_point.to_timespec(&timer_specification.it_value);

    if (::timerfd_settime(
            _pollable_file_descriptor.get_fd(),
            TFD_TIMER_ABSTIME | TFD_TIMER_CANCEL_ON_SET,
            &timer_specification,
            nullptr
        )
        == -1) {

        LOG_SYS_FATAL << "timerfd_settime failed";
    }
}

void Timerfd::cancel() {
    // all zero for cancelling the timer
    itimerspec timer_specification{};

    if (::timerfd_settime(
            _pollable_file_descriptor.get_fd(), 0, &timer_specification, nullptr
        )
        == -1) {

        LOG_SYS_FATAL << "timerfd_settime failed";
    }
}

uint64_t Timerfd::_retrieve_the_number_of_expirations() {
    uint64_t number_of_expirations;

    auto bytes_read = ::read(
        _pollable_file_descriptor.get_fd(),
        &number_of_expirations,
        sizeof(number_of_expirations)
    );

    if (bytes_read == -1) {
        if (errno == ECANCELED) {
            LOG_INFO << "a timer has been canceled due to discontinuous "
                        "changes to the real-time clock";
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

} // namespace xubinh_server