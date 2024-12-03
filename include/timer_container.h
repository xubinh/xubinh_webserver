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
    TimerIdentifier
    add_a_timer(const TimePoint &expiration_time_point, const Timer *timer_ptr);

    bool cancel_a_timer(const TimerIdentifier &timer_identifier);

    std::vector<Timer *> get_all_timers_expire_after_this_time_point(
        const TimePoint &expiration_time_point
    );

private:
    std::set<std::pair<TimePoint, const Timer *>> _timers;
    std::mutex _mutex;
};

} // namespace xubinh_server

#endif