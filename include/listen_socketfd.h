#ifndef XUBINH_SERVER_LISTEN_SOCKETFD
#define XUBINH_SERVER_LISTEN_SOCKETFD

#include <functional>

#include "inet_address.h"
#include "log_builder.h"
#include "pollable_file_descriptor.h"
#include "socketfd.h"

namespace xubinh_server {

class ListenSocketfd : public Socketfd {
public:
    using NewConnectionCallbackType = std::function<
        void(int connect_socketfd, const InetAddress &peer_address)>;

    static void set_socketfd_as_address_reusable(int socketfd);

    static void bind(int socketfd, const InetAddress &local_address);

    static void listen(int socketfd);

    ListenSocketfd(int fd, EventLoop *event_loop);

    ~ListenSocketfd() {
        _close_spare_fd();

        _pollable_file_descriptor.disable_read_event();
    }

    // used by internal framework
    void register_new_connection_callback(
        NewConnectionCallbackType new_connection_callback
    ) {
        _new_connection_callback = std::move(new_connection_callback);
    }

    void start() {
        if (!_new_connection_callback) {
            LOG_FATAL << "missing new connection callback";
        }

        _pollable_file_descriptor.enable_read_event();
    }

private:
    // simple wrapper for `::accept` with return values unchanged
    static int _accept_new_connection(
        int listen_socketfd, std::unique_ptr<InetAddress> &peer_address
    );

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