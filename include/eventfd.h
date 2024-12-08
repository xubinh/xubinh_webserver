#ifndef XUBINH_SERVER_EVENTFD
#define XUBINH_SERVER_EVENTFD

#include <sys/eventfd.h>

#include "log_builder.h"
#include "pollable_file_descriptor.h"

namespace xubinh_server {

class EventLoop;

class Eventfd : public PollableFileDescriptor {
public:
    Eventfd(EventLoop *event_loop)
        : PollableFileDescriptor(eventfd(0, _EVENTFD_FLAGS), event_loop) {

        if (_fd == -1) {
            LOG_SYS_FATAL << "failed creating eventfd";
        }
    }

    ~Eventfd() {
        ::close(_fd);
    }

    // write operation
    //
    // - thread-safe
    void increment_by_value(uint64_t value);

    // read operation
    //
    // - thread-safe
    uint64_t retrieve_the_sum();

private:
    // must be zero before (and including) Linux v2.6.26
    static constexpr int _EVENTFD_FLAGS = EFD_CLOEXEC | EFD_NONBLOCK;
};

} // namespace xubinh_server

#endif