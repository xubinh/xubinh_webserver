#ifndef XUBINH_SERVER_EVENTFD
#define XUBINH_SERVER_EVENTFD

#include <sys/eventfd.h>

#include "event_file_descriptor.h"
#include "log_builder.h"

namespace xubinh_server {

class EventLoop;

class Eventfd : public EventFileDescriptor {
public:
    Eventfd(EventLoop *event_loop)
        : EventFileDescriptor(eventfd(0, _EVENTFD_FLAGS), event_loop) {

        if (_fd == -1) {
            LOG_SYS_FATAL << "failed creating eventfd";
        }
    }

    ~Eventfd() {
        ::close(_fd);
    }

    void increment_by_value(uint64_t value);

    uint64_t retrieve_the_sum();

private:
    // must be zero before (and including) Linux v2.6.26
    static constexpr int _EVENTFD_FLAGS = EFD_CLOEXEC | EFD_NONBLOCK;
};

} // namespace xubinh_server

#endif