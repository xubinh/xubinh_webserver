#ifndef XUBINH_SERVER_FILE_DESCRIPTOR
#define XUBINH_SERVER_FILE_DESCRIPTOR

#include "event_dispatcher.h"

namespace xubinh_server {

class EventLoop;

// abstraction for edge-triggered non-blocking file descriptors
class PollableFileDescriptor : public EventDispatcher {
public:
    static void set_fd_as_nonblocking(int fd);

    PollableFileDescriptor(int fd, EventLoop *event_loop)
        : EventDispatcher(fd, event_loop), _fd(fd) {
    }

    ~PollableFileDescriptor() = default;

    int get_fd() {
        return _fd;
    }

protected:
    int _fd;
};

} // namespace xubinh_server

#endif