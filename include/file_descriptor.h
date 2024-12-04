#ifndef XUBINH_SERVER_FILE_DESCRIPTOR
#define XUBINH_SERVER_FILE_DESCRIPTOR

#include "event_dispatcher.h"

namespace xubinh_server {

class EventLoop;

class FileDescriptor {
public:
    static void set_fd_as_nonblocking(int fd);

    FileDescriptor(int fd, EventLoop *event_loop)
        : _fd(fd), _event_dispatcher(fd, event_loop) {
    }

    ~FileDescriptor() = default;

    int get_fd() {
        return _fd;
    }

protected:
    int _fd;
    EventDispatcher _event_dispatcher;
};

} // namespace xubinh_server

#endif