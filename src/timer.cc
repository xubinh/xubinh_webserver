#include "timer.h"

namespace xubinh_server {

bool Timer::expire_once() {
    LOG_DEBUG << "Timer::expire_once";

    LOG_DEBUG << "address: " << this
              << ", expiration time: "
                     + _expiration_time_point.to_datetime_string(
                         TimePoint::DatetimePurpose::PRINTING
                     )
                     + ", number of repetitions left: "
                     + std::to_string(_number_of_repetitions_left);

    LOG_DEBUG << "entering callback";

    _callback();

    LOG_DEBUG << "exiting callback";

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

bool Timer::expire_until(const TimePoint &time_point) {
    bool is_still_valid_after_last_time_of_expiration;

    while (true) {
        is_still_valid_after_last_time_of_expiration = expire_once();

        if (!is_still_valid_after_last_time_of_expiration
            || _expiration_time_point > time_point) {
            break;
        }
    }

    LOG_DEBUG << "Timer::expire_until: "
              << is_still_valid_after_last_time_of_expiration;

    return is_still_valid_after_last_time_of_expiration;
}

} // namespace xubinh_server