#include "tcp_client.h"

namespace xubinh_server {

void TcpClient::start() {
    if (_is_started) {
        return;
    }

    _is_started = true;

    if (!_message_callback) {
        LOG_FATAL << "missing message callback";
    }

    _preconnect_socketfd_ptr.reset(new PreconnectSocketfd(
        Socketfd::create_socketfd(),
        _loop,
        _server_address,
        _MAX_NUMBER_OF_RETRIES
    ));

    _preconnect_socketfd_ptr->register_new_connection_callback(std::bind(
        &TcpClient::_new_connection_callback, this, std::placeholders::_1
    ));
    _preconnect_socketfd_ptr->register_connect_fail_callback(
        std::move(_connect_fail_callback)
    );

    _preconnect_socketfd_ptr->start();
}

void TcpClient::_new_connection_callback(int connect_socketfd) {
    _preconnect_socketfd_ptr.reset();

    InetAddress local_address{connect_socketfd, InetAddress::LOCAL};
    InetAddress peer_address{connect_socketfd, InetAddress::PEER};

    _tcp_connect_socketfd_ptr = std::make_shared<TcpConnectSocketfd>(
        connect_socketfd, _loop, "0", local_address, peer_address
    );

    _tcp_connect_socketfd_ptr->register_message_callback(_message_callback);
    _tcp_connect_socketfd_ptr->register_write_complete_callback(
        _write_complete_callback
    );
    _tcp_connect_socketfd_ptr->register_close_callback(std::move(_close_callback
    ));

    _tcp_connect_socketfd_ptr->start();
}

} // namespace xubinh_server