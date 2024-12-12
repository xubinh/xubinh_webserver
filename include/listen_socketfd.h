#ifndef XUBINH_SERVER_LISTEN_SOCKETFD
#define XUBINH_SERVER_LISTEN_SOCKETFD

#include <functional>

#include "inet_address.h"
#include "log_builder.h"
#include "pollable_file_descriptor.h"

namespace xubinh_server {

class ListenSocketfd {
public:
    using NewConnectionCallbackType =
        std::function<void(int fd, const InetAddress &peer_address)>;

    ListenSocketfd(int fd, EventLoop *event_loop)
        : _pollable_file_descriptor(fd, event_loop) {

        if (fd < 0) {
            LOG_SYS_FATAL << "invalid file descriptor (must be non-negative)";
        }

        _pollable_file_descriptor.register_read_event_callback(
            std::bind(_read_event_callback, this)
        );

        _open_spare_fd();
    }

    ~ListenSocketfd() {
        _close_spare_fd();

        _pollable_file_descriptor.disable_read_event();
    }

    void register_new_connection_callback(
        NewConnectionCallbackType new_connection_callback
    ) {
        _new_connection_callback = std::move(new_connection_callback);
    }

    void start_reading() {
        if (!_new_connection_callback) {
            LOG_FATAL << "missing new connection callback";
        }

        _pollable_file_descriptor.enable_read_event();
    }

private:
    // simple wrapper for `::accept`, returns -1 for errors
    static int
    _accept_new_connection(int listen_socketfd, InetAddress &peer_address);

    void _open_spare_fd();

    void _close_spare_fd();

    // for dealing with special case where the process has reached the maximum
    // number of file descriptors allowed (RLIMIT_NOFILE)
    void _drop_connection_using_spare_fd();

    void _read_event_callback();

    int _spare_fd = -1;

    NewConnectionCallbackType _new_connection_callback;

    PollableFileDescriptor _pollable_file_descriptor;
};

} // namespace xubinh_server

#endif