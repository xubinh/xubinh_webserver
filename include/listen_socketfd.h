#ifndef __XUBINH_SERVER_LISTEN_SOCKETFD
#define __XUBINH_SERVER_LISTEN_SOCKETFD

#include <functional>

#include "inet_address.h"
#include "log_builder.h"
#include "pollable_file_descriptor.h"
#include "socketfd.h"
#include "util/time_point.h"

namespace xubinh_server {

class ListenSocketfd : public Socketfd {
public:
    using NewConnectionCallbackType = std::function<void(
        int connect_socketfd,
        const InetAddress &peer_address,
        util::TimePoint time_stamp
    )>;

    static void set_socketfd_as_address_reusable(int socketfd);

    static void bind(int socketfd, const InetAddress &local_address);

    static void listen(int socketfd);

    ListenSocketfd(int fd, EventLoop *event_loop, bool prefer_et = true);

    ~ListenSocketfd() {
        _close_spare_fd();

        _pollable_file_descriptor.detach_from_poller();

        _pollable_file_descriptor.close_fd();

        LOG_INFO << "exit destructor: ListenSocketfd";
    }

    // used by internal framework
    void register_new_connection_callback(
        NewConnectionCallbackType new_connection_callback
    ) {
        _new_connection_callback = std::move(new_connection_callback);
    }

    // void set_max_number_of_new_connections_at_a_time(
    //     size_t max_number_of_new_connections_at_a_time
    // ) {
    //     _max_number_of_new_connections_at_a_time =
    //         max_number_of_new_connections_at_a_time;
    // }

    void start() {
        if (!_new_connection_callback) {
            LOG_FATAL << "missing new connection callback";
        }

        _pollable_file_descriptor.register_read_event_callback(
            [this](util::TimePoint time_stamp) {
                _read_event_callback(time_stamp);
            }
        );

        _open_spare_fd();

        _pollable_file_descriptor.enable_read_event();
    }

private:
    // simple wrapper for `::accept` with return values unchanged
    static int _accept_new_connection(
        int listen_socketfd,
        std::unique_ptr<InetAddress> &peer_address,
        int flags
    );

    void _open_spare_fd();

    void _close_spare_fd();

    // for dealing with special case where the process has reached the maximum
    // number of file descriptors allowed (RLIMIT_NOFILE)
    void _drop_connection_using_spare_fd();

    void _read_event_callback(util::TimePoint time_stamp);

    int _spare_fd = -1;

    // size_t _max_number_of_new_connections_at_a_time = 64;

    // TCP socketfds are always at ET mode
    const int _accept_flags{SOCK_NONBLOCK | SOCK_CLOEXEC};

    NewConnectionCallbackType _new_connection_callback;

    PollableFileDescriptor _pollable_file_descriptor;
};

} // namespace xubinh_server

#endif