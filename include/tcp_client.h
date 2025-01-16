#ifndef __XUBINH_SERVER_TCP_CLIENT
#define __XUBINH_SERVER_TCP_CLIENT

#include "event_loop.h"
#include "inet_address.h"
#include "preconnect_socketfd.h"
#include "tcp_connect_socketfd.h"

namespace xubinh_server {

class TcpClient {
public:
    using PreconnectSocketfdPtr = PreconnectSocketfd::PreconnectSocketfdPtr;

    using TcpConnectSocketfdPtr = TcpConnectSocketfd::TcpConnectSocketfdPtr;

    using ConnectSuccessCallbackType =
        std::function<void(const TcpConnectSocketfdPtr &tcp_connect_socketfd_ptr
        )>;

    using MessageCallbackType = TcpConnectSocketfd::MessageCallbackType;

    using WriteCompleteCallbackType =
        TcpConnectSocketfd::WriteCompleteCallbackType;

    using ConnectFailCallbackType = PreconnectSocketfd::ConnectFailCallbackType;

    using CloseCallbackType = TcpConnectSocketfd::CloseCallbackType;
    ;

    TcpClient(EventLoop *loop, const InetAddress &server_address)
        : _loop(loop)
        , _server_address(server_address) {
    }

    ~TcpClient();

    void register_connect_success_callback(
        ConnectSuccessCallbackType connect_success_callback
    ) {
        _connect_success_callback = std::move(connect_success_callback);
    }

    void register_message_callback(MessageCallbackType message_callback) {
        _message_callback = std::move(message_callback);
    }

    void register_write_complete_callback(
        WriteCompleteCallbackType write_complete_callback
    ) {
        _write_complete_callback = std::move(write_complete_callback);
    }

    void
    register_connect_fail_callback(ConnectFailCallbackType connect_fail_callback
    ) {
        _connect_fail_callback = std::move(connect_fail_callback);
    }

    void register_close_callback(CloseCallbackType close_callback) {
        _close_callback = std::move(close_callback);
    }

    void start();

    void stop();

    void close_preconnect_socketfd();

    void close_tcp_connect_socketfd_ptr();

private:
    // for preconnect socketfd
    void
    _new_connection_callback(int connect_socketfd, util::TimePoint time_stamp);

    static constexpr int _MAX_NUMBER_OF_RETRIES = 5;

    bool _is_started = false;
    bool _is_stopped = false;

    ConnectSuccessCallbackType _connect_success_callback;

    MessageCallbackType _message_callback;
    WriteCompleteCallbackType _write_complete_callback;

    ConnectFailCallbackType _connect_fail_callback;

    CloseCallbackType _close_callback;

    EventLoop *_loop;

    const InetAddress &_server_address;

    PreconnectSocketfdPtr _preconnect_socketfd_ptr;

    TcpConnectSocketfdPtr _tcp_connect_socketfd_ptr;
};

} // namespace xubinh_server

#endif