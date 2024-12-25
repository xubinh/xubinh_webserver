#include "log_builder.h"

#include "../include/http_server.h"

namespace xubinh_server {

void HttpServer::start() {
    if (!_http_request_callback) {
        LOG_FATAL << "missing http request callback";
    }

    _tcp_server.register_connect_success_callback(
        [this](const TcpConnectSocketfdPtr &tcp_connect_socketfd_ptr) {
            _connect_success_callback_wrapper(tcp_connect_socketfd_ptr);
        }
    );

    _tcp_server.register_message_callback(
        [this](
            TcpConnectSocketfd *tcp_connect_socketfd_ptr,
            MutableSizeTcpBuffer *input_buffer,
            TimePoint time_stamp
        ) {
            _message_callback(
                tcp_connect_socketfd_ptr, input_buffer, time_stamp
            );
        }
    );

    // start the timer for removing inactive connections
    if (_connection_timeout_interval < TimeInterval{TimeInterval::FOREVER}) {
        LOG_TRACE << "register event -> main: remove_inactive_connections";

        _loop->run_after_time_interval(
            _connection_timeout_interval,
            _connection_timeout_interval,
            -1,
            [this]() {
                _remove_inactive_connections();
            }
        );
    }

    _tcp_server.start();
}

void HttpServer::_message_callback(
    TcpConnectSocketfd *tcp_connect_socketfd_ptr,
    MutableSizeTcpBuffer *input_buffer,
    TimePoint time_stamp
) {
    // marking active connections
    tcp_connect_socketfd_ptr->set_time_stamp(time_stamp);

    HttpParser &parser =
        util::any_cast<HttpParser &>(tcp_connect_socketfd_ptr->context);
    // HttpParser &parser =
    //     *util::any_cast<HttpParser *>(&tcp_connect_socketfd_ptr->context);

    bool is_success = parser.parse(*input_buffer, time_stamp);

    if (!is_success) {
        LOG_ERROR << "failed to parse HTTP request; connection abort";

        tcp_connect_socketfd_ptr->abort();

        return;
    }

    if (parser.is_success()) {
        const HttpRequest &request = parser.get_request();

        _http_request_callback(tcp_connect_socketfd_ptr, request);

        parser.reset();
    }
}

void HttpServer::_remove_inactive_connections() {
    LOG_TRACE << "enter event: remove_inactive_connections";

    LOG_TRACE << "current number of TCP connections: "
              << _tcp_server.get_number_of_tcp_connections();

    // draw a line to determine which connections are inactive
    _current_time_point = TimePoint();

    _tcp_server.run_for_each_connection(
        [this](const TcpConnectSocketfdPtr &tcp_connect_socketfd_ptr) {
            _check_and_remove_inactive_connection(tcp_connect_socketfd_ptr);
        }
    );
}

void HttpServer::_check_and_remove_inactive_connection(
    const TcpConnectSocketfdPtr &tcp_connect_socketfd_ptr
) {
    LOG_TRACE << "checking inactive tcp connection, id: "
              << tcp_connect_socketfd_ptr->get_id();

    TimePoint time_stamp = tcp_connect_socketfd_ptr->get_time_stamp();

    // first check if the connection is inactive in current loop
    if (_current_time_point - time_stamp > _connection_timeout_interval) {
        LOG_TRACE << "tcp connection might be inactive, id: "
                  << tcp_connect_socketfd_ptr->get_id();

        auto current_time_point = _current_time_point;
        auto connection_timeout_interval = _connection_timeout_interval;

        tcp_connect_socketfd_ptr->check_and_abort(
            // double check if the connection is still inactive in the
            // worker loop and actually abort it
            [current_time_point,
             connection_timeout_interval,
             &tcp_connect_socketfd_ptr]() {
                return (
                    current_time_point
                        - tcp_connect_socketfd_ptr->get_time_stamp()
                    > connection_timeout_interval
                );
            }
        );
    }
}

} // namespace xubinh_server