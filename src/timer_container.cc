#include "timer_container.h"

namespace xubinh_server {

TimerIdentifier TimerContainer::insert_a_timer(
    const TimePoint &expiration_time_point,
    const Timer *timer_ptr,
    TimePoint &earliest_expiration_time_point_before_insertion
) {
    {
        std::lock_guard<std::mutex> lock(_mutex);

        earliest_expiration_time_point_before_insertion =
            _timers.empty() ? TimePoint::FOREVER : _timers.begin()->first;

        _timers.insert({expiration_time_point, timer_ptr});
    }

    return {expiration_time_point, timer_ptr};
}

bool TimerContainer::remove_a_timer(
    const TimerIdentifier &timer_identifier,
    TimePoint &earliest_expiration_time_point_after_removal
) {
    const TimePoint &expiration_time_point =
        timer_identifier._expiration_time_point;
    const Timer *timer_ptr = timer_identifier._timer_ptr;

    int number_of_erased_timers = 0;

    {
        std::lock_guard<std::mutex> lock(_mutex);

        number_of_erased_timers =
            _timers.erase({expiration_time_point, timer_ptr});

        earliest_expiration_time_point_after_removal =
            _timers.empty() ? TimePoint::FOREVER : _timers.begin()->first;
    }

    return number_of_erased_timers > 0;
}

std::vector<Timer *>
TimerContainer::move_out_all_timers_expire_at_this_time_point(
    const TimePoint &expiration_time_points,
    TimePoint &earliest_expiration_time_point_after_moving_out
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

        earliest_expiration_time_point_after_moving_out =
            _timers.empty() ? TimePoint::FOREVER : _timers.begin()->first;
    }

    return timers_to_be_returned;
}

} // namespace xubinh_server
