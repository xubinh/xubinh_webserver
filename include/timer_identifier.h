#ifndef XUBINH_SERVER_TIMER_IDENTIFIER
#define XUBINH_SERVER_TIMER_IDENTIFIER

#include "timer.h"
#include "util/time_point.h"

namespace xubinh_server {

class TimerIdentifier {
public:
    TimerIdentifier(Timer *timer_ptr)
        : _timer_ptr(timer_ptr),
          _expiration_time_point(_timer_ptr->get_expiration_time_point()) {
    }

    friend class TimerContainer;

private:
    using TimePoint = util::TimePoint;

    Timer *_timer_ptr;
    TimePoint _expiration_time_point;
};

} // namespace xubinh_server

#endif