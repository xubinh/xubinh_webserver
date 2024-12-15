#include "tcp_server.h"

namespace xubinh_server {

void TcpServer::start() {
    if (_is_started) {
        return;
    }

    _is_started = true;

    if (!_message_callback) {
        LOG_FATAL << "missing message callback";
    }

    auto listen_socketfd = Socketfd::create_socketfd();

    ListenSocketfd::set_socketfd_as_address_reusable(listen_socketfd);
    ListenSocketfd::bind(listen_socketfd, _local_address);
    ListenSocketfd::listen(listen_socketfd);
    _listen_socketfd.reset(new ListenSocketfd(listen_socketfd, _loop));
    _listen_socketfd->register_new_connection_callback(std::bind(
        &TcpServer::_new_connection_callback,
        this,
        std::placeholders::_1,
        std::placeholders::_2
    ));

    _listen_socketfd->start();

    if (_thread_pool_capacity > 0) {
        _thread_pool.reset(new EventLoopThreadPool(
            _thread_pool_capacity, std::move(_thread_initialization_callback)
        ));

        _thread_pool->start();
    }
}

void TcpServer::stop() {
    if (_is_stopped) {
        return;
    }

    if (!_is_started) {
        LOG_FATAL << "tried to stop tcp server before starting it";
    }

    _is_stopped = true;

    if (_thread_pool_capacity > 0) {
        _thread_pool->stop();
    }
}

void TcpServer::_new_connection_callback(
    int connect_socketfd, const InetAddress &peer_address
) {
    std::string id = std::to_string(_tcp_connect_socketfds.size());
    EventLoop *loop =
        _thread_pool_capacity > 0 ? _thread_pool->get_next_loop() : _loop;
    InetAddress local_address{connect_socketfd, InetAddress::LOCAL};

    auto new_tcp_connect_socketfd_ptr = std::make_shared<TcpConnectSocketfd>(
        connect_socketfd, loop, id, local_address, peer_address
    );

    _tcp_connect_socketfds.insert(
        std::make_pair(id, new_tcp_connect_socketfd_ptr)
    );

    new_tcp_connect_socketfd_ptr->register_message_callback(_message_callback);
    new_tcp_connect_socketfd_ptr->register_write_complete_callback(
        _write_complete_callback
    );
    new_tcp_connect_socketfd_ptr->register_close_callback(
        std::bind(&TcpServer::_close_callback, this, std::placeholders::_1)
    );

    new_tcp_connect_socketfd_ptr->start();
}

void TcpServer::_close_callback(
    const TcpConnectSocketfdPtr &tcp_connect_socketfd_ptr
) {
    // must be executed inside the main loop
    _loop->run([=]() {
        _tcp_connect_socketfds.erase(tcp_connect_socketfd_ptr->get_id());
    });
}

} // namespace xubinh_server