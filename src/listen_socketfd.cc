#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "listen_socketfd.h"

namespace xubinh_server {

int ListenSocketfd::_accept_new_connection(
    int listen_socketfd, InetAddress &peer_address
) {
    using sockaddr_unknown_t = InetAddress::sockaddr_unknown_t;

    sockaddr_unknown_t peer_address_temp{};
    socklen_t peer_address_temp_len = sizeof(sockaddr_unknown_t);

    int connect_socketfd = accept(
        listen_socketfd,
        (struct sockaddr *)&peer_address_temp,
        &peer_address_temp_len
    );

    if (connect_socketfd >= 0) {
        peer_address = InetAddress(
            reinterpret_cast<sockaddr *>(&peer_address_temp),
            peer_address_temp_len
        );
    }

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

    InetAddress peer_address{};

    auto connect_socketfd = ListenSocketfd::_accept_new_connection(
        _pollable_file_descriptor.get_fd(), peer_address
    );

    ::close(connect_socketfd);

    _open_spare_fd();
}

void ListenSocketfd::_read_event_callback() {
    while (true) {
        InetAddress peer_address{};

        auto connect_socketfd = ListenSocketfd::_accept_new_connection(
            _pollable_file_descriptor.get_fd(), peer_address
        );

        // success
        if (connect_socketfd >= 0) {
            _new_connection_callback(connect_socketfd, peer_address);
        }

        // error
        else {
            // non-blocking listen socketfd with ET mode
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            // the connection was aborted by the peer; nothing needs to be done
            else if (errno == ECONNABORTED) {
                continue;
            }

            else if (errno == EMFILE) {
                LOG_SYS_ERROR << "too many open files (EMFILE)";

                _drop_connection_using_spare_fd();
            }

            else if (errno == ENFILE) {
                LOG_SYS_FATAL
                    << "system file descriptor limit reached (ENFILE)";
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

    LOG_SYS_FATAL << "failed to accept new connection";
}

} // namespace xubinh_server