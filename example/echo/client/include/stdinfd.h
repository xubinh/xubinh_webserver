#include "event_loop.h"
#include "tcp_connect_socketfd.h"
#include "util/time_point.h"

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
    }

    ~Stdinfd() {
        _pollable_file_descriptor.detach_from_poller();

        std::cout << "----------------- Session End -----------------"
                  << std::endl;

        _pollable_file_descriptor.close_fd();
    }

    void start() {
        _pollable_file_descriptor.register_read_event_callback(std::bind(
            &Stdinfd::_read_event_callback, this, std::placeholders::_1
        ));

        _pollable_file_descriptor.enable_read_event();

        std::cout << "---------------- Session Begin ----------------"
                  << std::endl;
    }

private:
    void _read_event_callback(xubinh_server::util::TimePoint time_stamp);

    std::weak_ptr<TcpConnectSocketfdPtr::element_type>
        _tcp_connect_socketfd_weak_ptr;

    xubinh_server::PollableFileDescriptor _pollable_file_descriptor;
};