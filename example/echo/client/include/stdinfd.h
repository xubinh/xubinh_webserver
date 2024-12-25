#ifndef __XUBINH_SERVER_STDINFD
#define __XUBINH_SERVER_STDINFD

#include "event_loop.h"
#include "tcp_connect_socketfd.h"
#include "util/time_point.h"

namespace xubinh_server {

using TcpConnectSocketfdPtr =
    xubinh_server::TcpConnectSocketfd::TcpConnectSocketfdPtr;

class Stdinfd {
public:
    Stdinfd(
        xubinh_server::EventLoop *loop,
        const TcpConnectSocketfdPtr &tcp_connect_socketfd_ptr
    );

    ~Stdinfd();

    void start();

private:
    void _read_event_callback(xubinh_server::util::TimePoint time_stamp);

    std::weak_ptr<TcpConnectSocketfdPtr::element_type>
        _tcp_connect_socketfd_weak_ptr;

    xubinh_server::PollableFileDescriptor _pollable_file_descriptor;
};

} // namespace xubinh_server

#endif