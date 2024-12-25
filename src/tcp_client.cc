#include "tcp_client.h"
#include "log_builder.h"

namespace xubinh_server {

TcpClient::~TcpClient() {
    if (!_is_started) {
        LOG_FATAL << "tried to destruct tcp client before starting it";
    }
}

void TcpClient::start() {
    if (_is_started) {
        return;
    }

    LOG_INFO << "starting TCP client...";

    if (!_message_callback) {
        LOG_FATAL << "missing message callback";
    }

    _preconnect_socketfd_ptr.reset(
        new PreconnectSocketfd(_loop, _server_address, _MAX_NUMBER_OF_RETRIES)
    );

    _preconnect_socketfd_ptr->register_new_connection_callback(
        [this](int connect_socketfd, util::TimePoint time_stamp) {
            _new_connection_callback(connect_socketfd, time_stamp);
        }
    );
    _preconnect_socketfd_ptr->register_connect_fail_callback(
        std::move(_connect_fail_callback)
    );

    _preconnect_socketfd_ptr->start();

    _is_started = true;

    LOG_INFO << "TCP client has started";
}

void TcpClient::stop() {
    if (_is_stopped) {
        return;
    }

    if (!_is_started) {
        LOG_FATAL << "tried to stop tcp client before starting it";
    }

    LOG_INFO << "stopping TCP server...";

    if (_preconnect_socketfd_ptr) {
        LOG_INFO << "cancelling connecting to the server...";

        _preconnect_socketfd_ptr->cancel();

        LOG_INFO << "finished cancelling connecting to the server";
    }

    if (_tcp_connect_socketfd_ptr) {
        LOG_INFO << "shutting down connection to the server...";

        _tcp_connect_socketfd_ptr->shutdown_write();

        LOG_INFO << "finished shutting down connection to the server";
    }

    _is_stopped = true;

    LOG_INFO << "TCP client has stopped";
}

void TcpClient::close_preconnect_socketfd() {
    LOG_TRACE << "register event -> main: close_preconnect_socketfd";

    _loop->run([this]() {
        LOG_TRACE << "enter event: close_preconnect_socketfd";

        _preconnect_socketfd_ptr.reset();
    });
}

void TcpClient::close_tcp_connect_socketfd_ptr() {
    LOG_TRACE << "register event -> main: close_tcp_connect_socketfd_ptr";

    _loop->run([this]() {
        LOG_TRACE << "enter event: close_tcp_connect_socketfd_ptr";

        _tcp_connect_socketfd_ptr.reset();
    });
}
void TcpClient::_new_connection_callback(
    int connect_socketfd, util::TimePoint time_stamp
) {
    _preconnect_socketfd_ptr.reset();

    InetAddress local_address{connect_socketfd, InetAddress::LOCAL};
    InetAddress peer_address{connect_socketfd, InetAddress::PEER};

    _tcp_connect_socketfd_ptr = std::make_shared<TcpConnectSocketfd>(
        connect_socketfd, _loop, 0, local_address, peer_address, time_stamp
    );

    if (_connect_success_callback) {
        _connect_success_callback(_tcp_connect_socketfd_ptr);
    }

    _tcp_connect_socketfd_ptr->register_message_callback(
        std::move(_message_callback)
    );
    _tcp_connect_socketfd_ptr->register_write_complete_callback(
        std::move(_write_complete_callback)
    );
    _tcp_connect_socketfd_ptr->register_close_callback(std::move(_close_callback
    ));

    _tcp_connect_socketfd_ptr->start();
}

} // namespace xubinh_server