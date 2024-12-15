#ifndef __XUBINH_SERVER_TIMER_IDENTIFIER
#define __XUBINH_SERVER_TIMER_IDENTIFIER

#include "timer.h"

namespace xubinh_server {

// for wrapping raw `Timer` pointers inside, providing a clean interface to the
// user of `EventLoop`
class TimerIdentifier {
public:
    explicit TimerIdentifier(const Timer *timer_ptr) : _timer_ptr(timer_ptr) {
    }

    friend class EventLoop;

private:
    const Timer *_timer_ptr;
};

} // namespace xubinh_server

#endif