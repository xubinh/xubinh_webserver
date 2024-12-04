#ifndef XUBINH_SERVER_TIMERFD
#define XUBINH_SERVER_TIMERFD

#include <sys/timerfd.h>

#include "event_file_descriptor.h"
#include "util/time_point.h"

namespace xubinh_server {

class EventLoop;

class Timerfd : public EventFileDescriptor {
private:
    using TimePoint = util::TimePoint;

public:
    Timerfd(EventLoop *event_loop)
        : EventFileDescriptor(
            timerfd_create(CLOCK_REALTIME, _TIMERFD_FLAGS), event_loop
        ) {

        if (_fd == -1) {
            LOG_SYS_FATAL << "failed creating timerfd";
        }
    }

    ~Timerfd() {
        ::close(_fd);
    }

    void set_alarm_at_time_point(const TimePoint &time_point);

    uint64_t retrieve_the_number_of_expirations();

    void cancel();

private:
    // must be zero before (and including) Linux v2.6.26
    static constexpr int _TIMERFD_FLAGS = TFD_CLOEXEC | TFD_NONBLOCK;
};

} // namespace xubinh_server

#endif