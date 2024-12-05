#ifndef XUBINH_SERVER_TIMER_CONTAINER
#define XUBINH_SERVER_TIMER_CONTAINER

#include <mutex>
#include <set>
#include <vector>

#include "timer.h"
#include "timer_identifier.h"
#include "util/time_point.h"

namespace xubinh_server {

class TimerContainer {
private:
    using TimePoint = util::TimePoint;

public:
    TimerIdentifier insert_a_timer(
        const TimePoint &expiration_time_point,
        const Timer *timer_ptr,
        TimePoint &earliest_expiration_time_point_before_insertion
    );

    bool remove_a_timer(
        const TimerIdentifier &timer_identifier,
        TimePoint &earliest_expiration_time_point_after_removal
    );

    std::vector<Timer *> move_out_all_timers_expire_at_this_time_point(
        const TimePoint &expiration_time_point,
        TimePoint &earliest_expiration_time_point_after_moving_out
    );

private:
    std::set<std::pair<TimePoint, const Timer *>> _timers;
    std::mutex _mutex;
};

} // namespace xubinh_server

#endif