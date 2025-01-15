#ifndef __XUBINH_SERVER_HTTP_SERVER
#define __XUBINH_SERVER_HTTP_SERVER

#include "http_parser.h"
#include "tcp_server.h"

namespace xubinh_server {

class HttpServer {
private:
    using TimePoint = util::TimePoint;
    using TimeInterval = util::TimeInterval;

public:
    using TcpConnectSocketfdPtr = TcpConnectSocketfd::TcpConnectSocketfdPtr;

    using ConnectSuccessCallbackType = TcpServer::ConnectSuccessCallbackType;

    using HttpRequestCallbackType = std::function<void(
        TcpConnectSocketfd *tcp_connect_socketfd_ptr,
        const HttpRequest &http_request
    )>;

    using WriteCompleteCallbackType = TcpServer::WriteCompleteCallbackType;

    using ThreadInitializationCallbackType =
        TcpServer::ThreadInitializationCallbackType;

    using RunForEachConnectionCallbackType =
        TcpServer::RunForEachConnectionCallbackType;

    HttpServer(EventLoop *loop, const InetAddress &local_address)
        : _loop(loop), _tcp_server(loop, local_address) {
    }

    ~HttpServer() = default;

    void register_connect_success_callback(
        ConnectSuccessCallbackType connect_success_callback
    ) {
        _connect_success_callback = std::move(connect_success_callback);
    }

    void
    register_http_request_callback(HttpRequestCallbackType http_request_callback
    ) {
        _http_request_callback = std::move(http_request_callback);
    }

    void register_write_complete_callback(
        WriteCompleteCallbackType write_complete_callback
    ) {
        _tcp_server.register_write_complete_callback(
            std::move(write_complete_callback)
        );
    }

    void set_thread_pool_capacity(size_t thread_pool_capacity) {
        _tcp_server.set_thread_pool_capacity(thread_pool_capacity);
    }

    void register_thread_initialization_callback(
        ThreadInitializationCallbackType thread_initialization_callback
    ) {
        _tcp_server.register_thread_initialization_callback(
            std::move(thread_initialization_callback)
        );
    }

    void start();

    void stop() {
        _tcp_server.stop();
    }

    void run_for_each_connection(RunForEachConnectionCallbackType callback) {
        _tcp_server.run_for_each_connection(std::move(callback));
    }

    void
    set_connection_timeout_interval(TimeInterval connection_timeout_interval) {
        _connection_timeout_interval = connection_timeout_interval;
    }

private:
    void _connect_success_callback_wrapper(
        const TcpConnectSocketfdPtr &tcp_connect_socketfd_ptr
    );

    void _message_callback(
        TcpConnectSocketfd *tcp_connect_socketfd_ptr,
        MutableSizeTcpBuffer *input_buffer,
        TimePoint time_stamp
    );

    void _remove_inactive_connections();

    void _check_and_remove_inactive_connection(
        const TcpConnectSocketfdPtr &tcp_connect_socketfd_ptr
    );

    EventLoop *_loop;

    TimePoint _current_time_point{};

    TimeInterval _connection_timeout_interval{60 * TimeInterval::SECOND};

    ConnectSuccessCallbackType _connect_success_callback;

    HttpRequestCallbackType _http_request_callback;

    TcpServer _tcp_server;
};

} // namespace xubinh_server

#endif