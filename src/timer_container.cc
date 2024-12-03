#include "timer_container.h"

namespace xubinh_server {

TimerIdentifier TimerContainer::add_a_timer(
    const TimePoint &expiration_time_point, const Timer *timer_ptr
) {
    {
        std::lock_guard<std::mutex> lock(_mutex);

        _timers.insert({expiration_time_point, timer_ptr});
    }

    return {expiration_time_point, timer_ptr};
}

bool TimerContainer::cancel_a_timer(const TimerIdentifier &timer_identifier) {
    const TimePoint &expiration_time_point =
        timer_identifier._expiration_time_point;
    const Timer *timer_ptr = timer_identifier._timer_ptr;

    int number_of_erased_timers = 0;

    {
        std::lock_guard<std::mutex> lock(_mutex);

        number_of_erased_timers =
            _timers.erase({expiration_time_point, timer_ptr});
    }

    return number_of_erased_timers > 0;
}

std::vector<Timer *>
TimerContainer::get_all_timers_expire_after_this_time_point(
    const TimePoint &expiration_time_points
) {
    std::vector<Timer *> timers_to_be_returned;

    {
        std::lock_guard<std::mutex> lock(_mutex);

        auto range_begin = _timers.begin();

        auto range_end = _timers.upper_bound(
            {expiration_time_points + static_cast<int64_t>(1), nullptr}
        );

        // optional for bidirectional iterator
        // timers_to_be_returned.reserve(std::distance(range_begin, range_end));

        for (auto it = range_begin; it != range_end; ++it) {
            timers_to_be_returned.push_back(const_cast<Timer *>(it->second));
        }

        _timers.erase(range_begin, range_end);
    }

    return timers_to_be_returned;
}

} // namespace xubinh_server
