#ifndef __XUBINH_SERVER_TCP_SERVER
#define __XUBINH_SERVER_TCP_SERVER

// #include <map>
#include <unordered_map>

#include "event_loop_thread_pool.h"
#include "listen_socketfd.h"
#include "tcp_connect_socketfd.h"

namespace xubinh_server {

class TcpServer {
public:
    using TcpConnectSocketfdPtr = TcpConnectSocketfd::TcpConnectSocketfdPtr;

    using ConnectSuccessCallbackType =
        std::function<void(const TcpConnectSocketfdPtr &tcp_connect_socketfd_ptr
        )>;

    using MessageCallbackType = TcpConnectSocketfd::MessageCallbackType;

    using WriteCompleteCallbackType =
        TcpConnectSocketfd::WriteCompleteCallbackType;

    using ThreadInitializationCallbackType =
        EventLoopThreadPool::ThreadInitializationCallbackType;

    using RunForEachConnectionCallbackType =
        std::function<void(const TcpConnectSocketfdPtr &tcp_connect_socketfd_ptr
        )>;

    TcpServer(EventLoop *loop, const InetAddress &local_address)
        : _loop(loop), _local_address(local_address) {
    }

    ~TcpServer();

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

    void set_thread_pool_capacity(size_t thread_pool_capacity) {
        _thread_pool_capacity = thread_pool_capacity;
    }

    void register_thread_initialization_callback(
        ThreadInitializationCallbackType thread_initialization_callback
    ) {
        _thread_initialization_callback =
            std::move(thread_initialization_callback);
    }

    void start();

    void stop();

    // runs in main loop
    void run_for_each_connection(RunForEachConnectionCallbackType callback) {
        LOG_TRACE << "register event -> main: run_for_each_connection";

        _loop->run([&]() {
            LOG_TRACE << "enter event: run_for_each_connection";

            for (const auto &pair : _tcp_connect_socketfds) {
                callback(pair.second);
            }
        });
    }

    size_t get_number_of_tcp_connections() const {
        return _tcp_connect_socketfds.size();
    }

private:
    // for listen socketfd
    void _new_connection_callback(
        int connect_socketfd, const InetAddress &peer_address
    );

    // for tcp connect socketfd
    void _close_callback(
        const TcpConnectSocketfdPtr &tcp_connect_socketfd_ptr,
        uint64_t functor_blocking_queue_index
    );

    bool _is_started = false;
    bool _is_stopped = false;

    ConnectSuccessCallbackType _connect_success_callback;

    MessageCallbackType _message_callback;
    WriteCompleteCallbackType _write_complete_callback;

    // event loop belongs to the outer thread, not the server
    EventLoop *_loop;

    const InetAddress _local_address;

    std::unique_ptr<ListenSocketfd> _listen_socketfd;

    // std::map<uint64_t, TcpConnectSocketfdPtr> _tcp_connect_socketfds;
    std::unordered_map<uint64_t, TcpConnectSocketfdPtr> _tcp_connect_socketfds;

    std::atomic<uint64_t> _tcp_connection_id_counter{0};

    size_t _thread_pool_capacity = 0;
    ThreadInitializationCallbackType _thread_initialization_callback;
    std::unique_ptr<EventLoopThreadPool> _thread_pool_ptr;
};

} // namespace xubinh_server

#endif