#include "timer.h"
#include "log_builder.h"

namespace xubinh_server {

Timer::Timer(
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
    LOG_TRACE << "Timer::Timer";

    LOG_TRACE << "address: " << this
              << ", expiration time: "
                     + _expiration_time_point.to_datetime_string(
                         TimePoint::Purpose::PRINTING
                     )
                     + ", number of repetitions left: "
                     + (_number_of_repetitions_left == -1
                            ? "∞"
                            : std::to_string(_number_of_repetitions_left));
}

bool Timer::expire_once() {
    LOG_TRACE << "added a timer, address: " << this
              << ", expiration time: "
                     + _expiration_time_point.to_datetime_string(
                         TimePoint::Purpose::PRINTING
                     )
                     + ", number of repetitions left: "
                     + (_number_of_repetitions_left == -1
                            ? "∞"
                            : std::to_string(_number_of_repetitions_left));

    LOG_TRACE << "entering callback";

    _callback();

    LOG_TRACE << "exiting callback";

    // an one-off timer, or a finite-repetition one just finished repeating
    if (!_repetition_time_interval || _number_of_repetitions_left == 0) {
        return false;
    }

    _expiration_time_point += _repetition_time_interval;

    // decrement repetition time for finite-repetition timer
    if (_number_of_repetitions_left > 0) {
        _number_of_repetitions_left -= 1;
    }

    return true;
}

bool Timer::expire_until(TimePoint time_point) {
    bool is_still_valid_after_last_time_of_expiration;

    while (true) {
        is_still_valid_after_last_time_of_expiration = expire_once();

        if (!is_still_valid_after_last_time_of_expiration
            || _expiration_time_point > time_point) {
            break;
        }
    }

    LOG_TRACE << "is still valid after expire_until: "
              << is_still_valid_after_last_time_of_expiration;

    return is_still_valid_after_last_time_of_expiration;
}

} // namespace xubinh_server