#include "timer.h"

namespace xubinh_server {

bool Timer::expire_once() {
    _callback();

    // one-off timer or finished repeating
    if (!_repetition_time_interval || _number_of_repetitions_left <= 0) {
        return false;
    }

    _expiration_time_point += _repetition_time_interval;

    if (_number_of_repetitions_left > 0) {
        _number_of_repetitions_left -= 1;
    }

    return true;
}

} // namespace xubinh_server