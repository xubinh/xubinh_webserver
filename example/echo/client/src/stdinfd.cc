#include <iostream>

#include "log_builder.h"

#include "../include/stdinfd.h"

namespace xubinh_server {

void Stdinfd::start() {
    _pollable_file_descriptor.register_read_event_callback(
        [this](xubinh_server::util::TimePoint time_stamp) {
            _read_event_callback(time_stamp);
        }
    );

    _pollable_file_descriptor.enable_read_event();

    std::cout << "---------------- Session Begin ----------------" << std::endl;
}

Stdinfd::Stdinfd(
    xubinh_server::EventLoop *loop,
    const TcpConnectSocketfdPtr &tcp_connect_socketfd_ptr
)
    : _tcp_connect_socketfd_weak_ptr(tcp_connect_socketfd_ptr)
    , _pollable_file_descriptor(STDIN_FILENO, loop) {
}

Stdinfd::~Stdinfd() {
    _pollable_file_descriptor.detach_from_poller();

    std::cout << "----------------- Session End -----------------" << std::endl;

    _pollable_file_descriptor.close_fd();
}

void Stdinfd::_read_event_callback(__attribute__((unused))
                                   xubinh_server::util::TimePoint time_stamp) {
    LOG_TRACE << "stdin read event encountered";

    auto _tcp_connect_socketfd_ptr = _tcp_connect_socketfd_weak_ptr.lock();

    if (!_tcp_connect_socketfd_ptr) {
        LOG_ERROR << "connection closed";

        _pollable_file_descriptor.disable_read_event();

        return;
    }

    while (true) {
        char buffer[256];

        ssize_t bytes_read = ::read(STDIN_FILENO, buffer, sizeof(buffer));

        if (bytes_read > 0) {
            _tcp_connect_socketfd_ptr->send(buffer, bytes_read);

            LOG_DEBUG << "send message to server: "
                      << std::string(
                             buffer, static_cast<size_t>(bytes_read) - 1
                         );

            continue;
        }

        else if (bytes_read == 0) {
            LOG_FATAL << "stdin closed";
        }

        else {
            if (errno == EAGAIN) {
                break;
            }

            else {
                LOG_SYS_FATAL << "unknown error when reading from stdin";
            }
        }
    }
}

} // namespace xubinh_server