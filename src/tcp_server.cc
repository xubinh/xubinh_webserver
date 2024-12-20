#include "tcp_server.h"
#include "log_collector.h"

namespace xubinh_server {

TcpServer::~TcpServer() {
    if (!_is_stopped) {
        LOG_SYS_FATAL << "tried to destruct tcp server before stopping it";
    }

    LOG_INFO << "exit destructor: TcpServer";

    // for the case where worker threads got blocked and thread pool
    // couldn't get destroyed
    LogCollector::flush();
}

void TcpServer::start() {
    if (_is_started) {
        return;
    }

    LOG_INFO << "starting TCP server...";

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
    // _listen_socketfd->set_max_number_of_new_connections_at_a_time(
    //     std::max(_thread_pool_capacity, static_cast<size_t>(1))
    // );

    _listen_socketfd->start();

    if (_thread_pool_capacity > 0) {
        _thread_pool.reset(new EventLoopThreadPool(
            _thread_pool_capacity, std::move(_thread_initialization_callback)
        ));

        _thread_pool->start();
    }

    _is_started = true;

    LOG_INFO << "TCP server has started";
}

void TcpServer::stop() {
    if (_is_stopped) {
        return;
    }

    if (!_is_started) {
        LOG_FATAL << "tried to stop tcp server before starting it";
    }

    LOG_INFO << "stopping TCP server...";

    LOG_INFO << "stopping listening for new connections...";

    // stop accepting new connections
    _listen_socketfd.reset();

    LOG_INFO << "finished stopping listening for new connections";

    if (_thread_pool_capacity > 0) {
        LOG_INFO << "shutting down thread pool";

        _thread_pool->stop();

        LOG_INFO << "finished shutting down thread pool";
    }

    _is_stopped = true;

    LOG_INFO << "TCP server has stopped";
}

void TcpServer::_new_connection_callback(
    int connect_socketfd, const InetAddress &peer_address
) {
    uint64_t id =
        _tcp_connection_id_counter.fetch_add(1, std::memory_order_relaxed);
    EventLoop *loop =
        _thread_pool_capacity > 0 ? _thread_pool->get_next_loop() : _loop;
    InetAddress local_address{connect_socketfd, InetAddress::LOCAL};

    auto new_tcp_connect_socketfd_ptr = std::make_shared<TcpConnectSocketfd>(
        connect_socketfd, loop, id, local_address, peer_address
    );

    if (_connect_success_callback) {
        _connect_success_callback(new_tcp_connect_socketfd_ptr);
    }

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
    LOG_TRACE << "register event -> main: _close_callback";

    // must be executed inside the main loop
    _loop->run([this, tcp_connect_socketfd_ptr]() {
        LOG_TRACE << "enter event: _close_callback";

        _tcp_connect_socketfds.erase(tcp_connect_socketfd_ptr->get_id());

        LOG_TRACE << "erase TCP connection, id: "
                  << tcp_connect_socketfd_ptr->get_id();

        LOG_DEBUG << "number of reference: "
                  << tcp_connect_socketfd_ptr.use_count();
    });
}

} // namespace xubinh_server