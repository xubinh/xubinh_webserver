#ifndef __XUBINH_SERVER_TIMER
#define __XUBINH_SERVER_TIMER

#include <algorithm>
#include <functional>

#include "log_builder.h"
#include "util/time_point.h"

namespace xubinh_server {

// not thread-safe
class Timer {
private:
    using TimePoint = util::TimePoint;
    using TimeInterval = util::TimeInterval;

public:
    using TimerCallbackType = std::function<void()>;

    Timer(
        const TimePoint &expiration_time_point,
        const TimeInterval &repetition_time_interval,
        int number_of_repetitions_left,
        TimerCallbackType callback
    )
        : _expiration_time_point(expiration_time_point),
          _repetition_time_interval(std::max(
              static_cast<int64_t>(0), repetition_time_interval.nanoseconds
          )),
          _number_of_repetitions_left(std::max(-1, number_of_repetitions_left)),
          _callback(std::move(callback)) {
        LOG_TRACE << "Timer::Timer";

        LOG_DEBUG << "address: " << this
                  << ", expiration time: "
                         + _expiration_time_point.to_datetime_string(
                             TimePoint::Purpose::PRINTING
                         )
                         + ", number of repetitions left: "
                         + (_number_of_repetitions_left == -1
                                ? "∞"
                                : std::to_string(_number_of_repetitions_left));
    }

    // no copying
    Timer(const Timer &) = delete;
    Timer &operator=(const Timer &) = delete;

    // no moving
    Timer(Timer &&) = delete;
    Timer &operator=(Timer &&) = delete;

    // - expire this timer once
    // - return true if the timer is still valid after this expiration. or false
    // if otherwise
    bool expire_once();

    // expire this timer until the next expiration time point comes strictly
    // after the given one, or until the timer is no longer valid
    bool expire_until(TimePoint time_point);

    TimePoint get_expiration_time_point() const {
        return _expiration_time_point;
    }

private:
    TimePoint _expiration_time_point;
    TimeInterval _repetition_time_interval; // 0 for one-off timer
    int _number_of_repetitions_left;        // -1 for infinite repetition
    TimerCallbackType _callback;
};

} // namespace xubinh_server

#endif