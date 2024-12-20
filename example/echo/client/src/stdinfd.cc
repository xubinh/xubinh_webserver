#include "../include/stdinfd.h"

namespace xubinh_server {

void Stdinfd::_read_event_callback(xubinh_server::util::TimePoint time_stamp) {
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

            LOG_DEBUG << "send message: "
                             + std::string(buffer, buffer + bytes_read - 1);

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