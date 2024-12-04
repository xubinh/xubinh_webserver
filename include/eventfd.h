#ifndef XUBINH_SERVER_EVENTFD
#define XUBINH_SERVER_EVENTFD

#include <sys/eventfd.h>

#include "file_descriptor.h"
#include "log_builder.h"

namespace xubinh_server {

class EventLoop;

class Eventfd : public EventFileDescriptor {
public:
    Eventfd(EventLoop *event_loop, unsigned int initial_value)
        : EventFileDescriptor(
            eventfd(initial_value, _EVENTFD_FLAGS), event_loop
        ) {

        if (_fd == -1) {
            LOG_SYS_FATAL << "eventfd failed";
        }
    }

    ~Eventfd() {
        ::close(_fd);
    }

    void increment_by_value(uint64_t value);

    uint64_t retrieve_the_sum();

private:
    // must be zero before Linux v2.6.26
    static constexpr int _EVENTFD_FLAGS = EFD_CLOEXEC | EFD_NONBLOCK;
};

} // namespace xubinh_server

#endif