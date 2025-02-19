#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "listen_socketfd.h"
#include "log_builder.h"

namespace xubinh_server {

void ListenSocketfd::set_socketfd_as_address_reusable(int socketfd) {
    int set = 1;

    if (::setsockopt(
            socketfd,
            SOL_SOCKET,
            SO_REUSEADDR,
            &set,
            static_cast<socklen_t>(sizeof set)
        )
        == -1) {

        LOG_SYS_FATAL << "failed when setting SO_REUSEADDR to a socketfd";
    }
}

void ListenSocketfd::bind(int socketfd, const InetAddress &local_address) {
    if (::bind(
            socketfd,
            local_address.get_address(),
            local_address.get_address_length()
        )
        == -1) {

        LOG_SYS_FATAL << "failed when binding a socketfd";
    }
}

void ListenSocketfd::listen(int socketfd) {
    if (::listen(socketfd, SOMAXCONN) == -1) {
        LOG_SYS_FATAL << "failed when start listening a socketfd";
    }
}

ListenSocketfd::ListenSocketfd(int fd, EventLoop *event_loop)
    : _pollable_file_descriptor(fd, event_loop, false, true) {
}

ListenSocketfd::~ListenSocketfd() {
    _close_spare_fd();

    _pollable_file_descriptor.detach_from_poller();

    _pollable_file_descriptor.close_fd();

    LOG_INFO << "exit destructor: ListenSocketfd";
}

void ListenSocketfd::start() {
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

int ListenSocketfd::_accept_new_connection(
    int listen_socketfd,
    std::unique_ptr<InetAddress> &peer_address_ptr,
    int flags
) {
    sockaddr_storage peer_address_temp{};
    socklen_t peer_address_temp_len = sizeof(sockaddr_storage);

    int connect_socketfd = ::accept4(
        listen_socketfd,
        reinterpret_cast<sockaddr *>(&peer_address_temp),
        &peer_address_temp_len,
        flags
    );

    if (connect_socketfd >= 0) {
        peer_address_ptr.reset(new InetAddress(
            reinterpret_cast<sockaddr *>(&peer_address_temp),
            peer_address_temp_len
        ));
    }

    // may fail and return errno instead
    return connect_socketfd;
}

void ListenSocketfd::_open_spare_fd() {
    _spare_fd = ::open("/dev/null", O_RDONLY);

    if (_spare_fd == -1) {
        LOG_SYS_FATAL << "unable to open spare fd";
    }
}

void ListenSocketfd::_close_spare_fd() {
    ::close(_spare_fd);
}

void ListenSocketfd::_drop_connection_using_spare_fd() {
    _close_spare_fd();

    std::unique_ptr<InetAddress> peer_address_ptr;

    auto connect_socketfd = ListenSocketfd::_accept_new_connection(
        _pollable_file_descriptor.get_fd(), peer_address_ptr, 0
    );

    if (connect_socketfd < 0) {
        LOG_FATAL
            << "still failed to accept new connections after dropping spare fd";
    }

    ::close(connect_socketfd);

    _open_spare_fd();
}

void ListenSocketfd::_read_event_callback(util::TimePoint time_stamp) {
    LOG_TRACE << "entering ListenSocketfd::_read_event_callback";

    for (int i = 0; i < _max_number_of_new_connections_at_a_time; i++) {
        LOG_TRACE << "accepting next connection...";

        std::unique_ptr<InetAddress> peer_address_ptr;

        auto connect_socketfd = ListenSocketfd::_accept_new_connection(
            _pollable_file_descriptor.get_fd(), peer_address_ptr, _accept_flags
        );

        // success
        if (connect_socketfd >= 0) {
            LOG_TRACE << "connection OK, connect socketfd: "
                      << connect_socketfd;

            _new_connection_callback(
                connect_socketfd, *peer_address_ptr, time_stamp
            );
        }

        // error
        else /* if (connect_socketfd == -1) */ {
            LOG_TRACE << "connection error";

            // non-blocking listen socketfd
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                LOG_TRACE << "error: EAGAIN; breaking loop";

                break;
            }

            // the connection was aborted by the peer; nothing needs to be done
            else if (errno == ECONNABORTED) {
                LOG_TRACE << "error: ECONNABORTED; continue looping";

                continue;
            }

            else if (errno == EMFILE || errno == ENFILE) {
                // LOG_SYS_ERROR << "too many open files (EMFILE)";

                // _drop_connection_using_spare_fd();

                // continue;

                LOG_TRACE << "error: EMFILE/ENFILE; breaking loop";

                // just heads for eventfd and closes all the pending connections
                break;
            }

            // [NOTE]: signals will be managed by the user using Signalfd, so
            // here just make it abort
            // else if(errno == EINTR){
            //     continue;
            // }

            else {
                LOG_SYS_FATAL << "unknown error";
            }
        }
    }

    LOG_TRACE << "exiting ListenSocketfd::_read_event_callback";
}

int ListenSocketfd::_max_number_of_new_connections_at_a_time = 1000;

} // namespace xubinh_server