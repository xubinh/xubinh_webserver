#ifndef XUBINH_SERVER_TIMER
#define XUBINH_SERVER_TIMER

#include <algorithm>
#include <functional>

#include "util/time_point.h"

namespace xubinh_server {

class Timer {
private:
    using TimePoint = util::TimePoint;
    using TimeInterval = util::TimeInterval;

public:
    using TimerCallbackType = std::function<void()>;

    Timer(
        TimePoint expiration_time_point,
        TimeInterval repetition_time_interval,
        int number_of_repetitions_left,
        TimerCallbackType callback
    )
        : _expiration_time_point(expiration_time_point),
          _repetition_time_interval(std::max(
              static_cast<int64_t>(0), repetition_time_interval.nanoseconds
          )),
          _number_of_repetitions_left(std::max(-1, number_of_repetitions_left)),
          _callback(std::move(callback)) {
    }

    // no copying
    Timer(const Timer &) = delete;
    Timer &operator=(const Timer &) = delete;

    // no moving
    Timer(Timer &&) = delete;
    Timer &operator=(Timer &&) = delete;

    // not thread-safe
    bool expire_once();

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