#include "tcp_server.h"
#include "log_builder.h"
#include "log_collector.h"

namespace xubinh_server {

TcpServer::~TcpServer() {
    if (!_is_stopped) {
        LOG_SYS_FATAL << "tried to destruct tcp server before stopping it";
    }

    _thread_pool_ptr.reset();

    if (!_tcp_connect_socketfds.empty()) {
        LOG_WARN << "TCP server stopped when there are still connections "
                    "left, closing them abruptly...";

        for (auto &pair : _tcp_connect_socketfds) {
            _close_callback(&(*pair.second), 0);
        }
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
    _listen_socketfd->register_new_connection_callback(
        [this](
            int connect_socketfd,
            const InetAddress &peer_address,
            util::TimePoint time_stamp
        ) {
            _new_connection_callback(
                connect_socketfd, peer_address, time_stamp
            );
        }
    );
    // _listen_socketfd->set_max_number_of_new_connections_at_a_time(
    //     std::max(_thread_pool_capacity, static_cast<size_t>(1))
    // );

    _listen_socketfd->start();

    if (_thread_pool_capacity > 0) {
        _thread_pool_ptr.reset(new EventLoopThreadPool(_thread_pool_capacity));

        if (_thread_initialization_callback) {
            _thread_pool_ptr->register_thread_initialization_callback(
                _thread_initialization_callback
            );
        }

        _thread_pool_ptr->start();
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

        _thread_pool_ptr->stop();

        LOG_INFO << "finished shutting down thread pool";
    }

    _is_stopped = true;

    LOG_INFO << "TCP server has stopped";
}

void TcpServer::run_for_each_connection(
    const RunForEachConnectionCallbackType &callback
) {
    LOG_TRACE << "register event -> main: run_for_each_connection";

    _loop->run([=]() {
        LOG_TRACE << "enter event: run_for_each_connection";

        for (const auto &pair : _tcp_connect_socketfds) {
            callback(pair.second);
        }
    });
}

void TcpServer::run_for_each_connection(
    RunForEachConnectionCallbackType &&callback
) {
    LOG_TRACE << "register event -> main: run_for_each_connection";

    auto callback_wrapper = [this](RunForEachConnectionCallbackType &callback) {
        LOG_TRACE << "enter event: run_for_each_connection";

        for (const auto &pair : _tcp_connect_socketfds) {
            callback(pair.second);
        }
    };

    _loop->run(std::bind(std::move(callback_wrapper), std::move(callback)));
}

void TcpServer::_new_connection_callback(
    int connect_socketfd,
    const InetAddress &peer_address,
    util::TimePoint time_stamp
) {
    uint64_t id =
        _tcp_connection_id_counter.fetch_add(1, std::memory_order_relaxed);
    LOG_TRACE << "Does connection (id: " << id << ") already exist: "
              << (_tcp_connect_socketfds.find(id)
                  != _tcp_connect_socketfds.end());
    EventLoop *loop =
        _thread_pool_capacity > 0 ? _thread_pool_ptr->get_next_loop() : _loop;
    InetAddress local_address{connect_socketfd, InetAddress::LOCAL};

    auto insert_result = _tcp_connect_socketfds.emplace(
        id,
        // std::make_shared<TcpConnectSocketfd>(
        //     connect_socketfd, loop, id, local_address, peer_address,
        //     time_stamp
        // )
        std::allocate_shared<TcpConnectSocketfd>(
            // util::StaticSemiLockFreeSlabAllocator<TcpConnectSocketfd>(),
            util::StaticThreadLocalSlabAllocator<TcpConnectSocketfd>(),
            connect_socketfd,
            loop,
            id,
            local_address,
            peer_address,
            time_stamp
        )
    );

    if (!insert_result.second) {
        LOG_FATAL << "execution flow never reaches here (in real world case)";
    }

    auto &new_tcp_connect_socketfd_ptr = insert_result.first->second;

    if (_connect_success_callback) {
        _connect_success_callback(new_tcp_connect_socketfd_ptr);
    }

    new_tcp_connect_socketfd_ptr->register_message_callback(_message_callback);
    new_tcp_connect_socketfd_ptr->register_write_complete_callback(
        _write_complete_callback
    );
    auto loop_index = loop->get_loop_index();
    new_tcp_connect_socketfd_ptr->register_close_callback(
        [this, loop_index](TcpConnectSocketfd *tcp_connect_socketfd_ptr) {
            _close_callback(tcp_connect_socketfd_ptr, loop_index);
        }
    );

    new_tcp_connect_socketfd_ptr->start();

    if (id % 10000 == 0) {
        LOG_DEBUG << "TCP connection establishment checkpoint, id: " << id;
    }
}

void TcpServer::_close_callback(
    TcpConnectSocketfd *tcp_connect_socketfd_ptr,
    uint64_t functor_blocking_queue_index
) {
    LOG_TRACE << "register event -> main: _close_callback";

    auto connection_id = tcp_connect_socketfd_ptr->get_id();

    // must be executed inside the main loop
    _loop->run(
        [this, connection_id]() {
            LOG_TRACE << "enter event: _close_callback";

            LOG_TRACE << "TCP connection use count: "
                      << _tcp_connect_socketfds[connection_id].use_count();

            _tcp_connect_socketfds.erase(connection_id);

            LOG_TRACE << "erase TCP connection, id: " << connection_id;
        },
        functor_blocking_queue_index
    );
}

} // namespace xubinh_server