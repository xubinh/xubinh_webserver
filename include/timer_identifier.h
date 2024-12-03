#ifndef XUBINH_SERVER_TIMER_IDENTIFIER
#define XUBINH_SERVER_TIMER_IDENTIFIER

#include "timer.h"
#include "util/time_point.h"

namespace xubinh_server {

class TimerIdentifier {
private:
    using TimePoint = util::TimePoint;

public:
    TimerIdentifier(
        const TimePoint &expiration_time_point, const Timer *timer_ptr
    )
        : _expiration_time_point(expiration_time_point), _timer_ptr(timer_ptr) {
    }

    friend class TimerContainer;

private:
    TimePoint _expiration_time_point;
    const Timer *_timer_ptr;
};

} // namespace xubinh_server

#endif