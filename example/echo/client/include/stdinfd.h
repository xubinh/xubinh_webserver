#include "event_loop.h"
#include "tcp_connect_socketfd.h"

using TcpConnectSocketfdPtr =
    xubinh_server::TcpConnectSocketfd::TcpConnectSocketfdPtr;

class Stdinfd {
public:
    Stdinfd(
        xubinh_server::EventLoop *loop,
        const TcpConnectSocketfdPtr &tcp_connect_socketfd_ptr
    )
        : _tcp_connect_socketfd_weak_ptr(tcp_connect_socketfd_ptr),
          _pollable_file_descriptor(STDIN_FILENO, loop) {

        _pollable_file_descriptor.register_read_event_callback(
            std::bind(&Stdinfd::_read_event_callback, this)
        );
    }

    ~Stdinfd() {
        _pollable_file_descriptor.detach_from_poller();

        ::close(_pollable_file_descriptor.get_fd());
    }

    void start() {
        _pollable_file_descriptor.enable_read_event();
    }

private:
    void _read_event_callback();

    std::weak_ptr<TcpConnectSocketfdPtr::element_type>
        _tcp_connect_socketfd_weak_ptr;

    xubinh_server::PollableFileDescriptor _pollable_file_descriptor;
};