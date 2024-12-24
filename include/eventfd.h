#ifndef __XUBINH_SERVER_EVENTFD
#define __XUBINH_SERVER_EVENTFD

#include <sys/eventfd.h>

#include "log_builder.h"
#include "pollable_file_descriptor.h"
#include "util/time_point.h"

namespace xubinh_server {

class EventLoop;

class Eventfd {
public:
    using MessageCallbackType = std::function<void(uint64_t)>;

    static int create_eventfd(int flags) {
        if (flags == 0) {
            flags = _DEFAULT_EVENTFD_FLAGS;
        }

        return ::eventfd(0, flags);
    }

    Eventfd(int fd, EventLoop *event_loop)
        : _pollable_file_descriptor(fd, event_loop) {
    }

    ~Eventfd();

    void register_eventfd_message_callback(
        MessageCallbackType eventfd_message_callback
    ) {
        _message_callback = std::move(eventfd_message_callback);
    }

    void start() {
        if (!_message_callback) {
            LOG_FATAL << "missing message callback";
        }

        _pollable_file_descriptor.register_read_event_callback(
            [this](util::TimePoint time_stamp) {
                _read_event_callback(time_stamp);
            }
        );

        _pollable_file_descriptor.enable_read_event();
    }

    // write operation
    //
    // - thread-safe
    void increment_by_value(uint64_t value);

private:
    // must be zero before (and including) Linux v2.6.26
    static constexpr int _DEFAULT_EVENTFD_FLAGS = EFD_CLOEXEC | EFD_NONBLOCK;

    void _read_event_callback(util::TimePoint time_stamp) {
        _message_callback(_retrieve_the_sum());
    }

    // read operation
    //
    // - thread-safe
    uint64_t _retrieve_the_sum();

    MessageCallbackType _message_callback;

    PollableFileDescriptor _pollable_file_descriptor;
};

} // namespace xubinh_server

#endif