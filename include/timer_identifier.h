#ifndef XUBINH_SERVER_TIMER_IDENTIFIER
#define XUBINH_SERVER_TIMER_IDENTIFIER

#include "timer.h"
#include "timer_container.h"

namespace xubinh_server {

class TimerIdentifier {
public:
    TimerIdentifier(Timer *timer) : _timer(timer) {
    }

    friend class TimerContainer;

private:
    Timer *_timer;
};

} // namespace xubinh_server

#endif