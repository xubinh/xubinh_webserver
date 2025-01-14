#ifndef __XUBINH_SERVER_TIMER_CONTAINER
#define __XUBINH_SERVER_TIMER_CONTAINER

#include <mutex>
#include <set>
#include <vector>

#include "timer.h"
#include "util/time_point.h"

namespace xubinh_server {

// not thread-safe
class TimerContainer {
private:
    using TimePoint = util::TimePoint;

public:
    TimePoint get_earliest_expiration_time_point() {
        return _timers.empty() ? TimePoint::FOREVER : _timers.begin()->first;
    }

    bool empty() const {
        return _timers.empty();
    }

    size_t size() const {
        return _timers.size();
    }

    void insert_one(const Timer *timer_ptr);

    void insert_all(const std::vector<const Timer *> &timers);

    bool remove_one(const Timer *timer_ptr);

    // move out all the timers that are supposed to expire before or right at
    // the given time point
    std::vector<const Timer *>
    move_out_before_or_at(TimePoint expiration_time_point);

private:
    std::set<std::pair<TimePoint, const Timer *>> _timers;
};

} // namespace xubinh_server

#endif