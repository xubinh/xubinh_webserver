#include "timer_container.h"

namespace xubinh_server {

void TimerContainer::insert_one(const Timer *timer_ptr) {
    const auto &expiration_time_point = timer_ptr->get_expiration_time_point();

    _timers.insert({expiration_time_point, timer_ptr});
}

void TimerContainer::insert_all(const std::vector<const Timer *> &timers) {
    for (const Timer *timer_ptr : timers) {
        const auto &expiration_time_point =
            timer_ptr->get_expiration_time_point();

        _timers.insert({expiration_time_point, timer_ptr});
    }
}

bool TimerContainer::remove_one(const Timer *timer_ptr) {
    TimePoint expiration_time_point = timer_ptr->get_expiration_time_point();

    const int &number_of_removed_timers =
        static_cast<int>(_timers.erase({expiration_time_point, timer_ptr}));

    return static_cast<bool>(number_of_removed_timers);
}

std::vector<const Timer *>
TimerContainer::move_out_before_or_at(TimePoint expiration_time_point) {
    std::vector<const Timer *> timers_to_be_returned;

    auto range_begin = _timers.begin();

    auto range_end = _timers.lower_bound(
        {expiration_time_point + static_cast<int64_t>(1), nullptr}
    );

    // optional for bidirectional iterator
    // timers_to_be_returned.reserve(std::distance(range_begin, range_end));

    for (auto it = range_begin; it != range_end; it++) {
        timers_to_be_returned.push_back(it->second);
    }

    _timers.erase(range_begin, range_end);

    return timers_to_be_returned;
}

} // namespace xubinh_server
