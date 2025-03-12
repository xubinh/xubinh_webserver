#ifndef __XUBINH_SERVER_TCP_SERVER
#define __XUBINH_SERVER_TCP_SERVER

#include <map>

#include "event_loop_thread_pool.h"
#include "listen_socketfd.h"
#include "tcp_connect_socketfd.h"
#include "util/mutex.h"
#include "util/slab_allocator.h"

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
        : _loop(loop)
        , _local_address(local_address) {
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
    void
    run_for_each_connection(const RunForEachConnectionCallbackType &callback);

    // runs in main loop
    void run_for_each_connection(RunForEachConnectionCallbackType &&callback);

    size_t get_number_of_tcp_connections() const {
        return _tcp_connect_socketfds.size();
    }

private:
    // for listen socketfd
    void _new_connection_callback(
        int connect_socketfd,
        const InetAddress &peer_address,
        util::TimePoint time_stamp
    );

    // for tcp connect socketfd
    void _close_callback(
        TcpConnectSocketfd *tcp_connect_socketfd_ptr,
        size_t functor_blocking_queue_index
    );

    void _transfer_tcp_connections_to_background_thread();

#ifdef __USE_SHARED_PTR_DESTRUCTION_TRANSFERING
    static constexpr const size_t
        _MAX_SIZE_OF_CURRENT_VECTOR_OF_TCP_CONNECT_SOCKETFDS_TO_BE_DESTROYED =
            512;
#endif

    bool _is_started = false;
    bool _is_stopped = false;

    ConnectSuccessCallbackType _connect_success_callback;

    MessageCallbackType _message_callback;
    WriteCompleteCallbackType _write_complete_callback;

    // event loop belongs to the outer thread, not the server
    EventLoop *_loop;

    const InetAddress _local_address;

    std::unique_ptr<ListenSocketfd> _listen_socketfd;

    std::map<
        uint64_t,
        TcpConnectSocketfdPtr,
        std::less<uint64_t>,
        util::SimpleSlabAllocator<
            std::pair<const uint64_t, TcpConnectSocketfdPtr>>>
        _tcp_connect_socketfds;

#ifdef __USE_SHARED_PTR_DESTRUCTION_TRANSFERING
    std::vector<std::vector<std::shared_ptr<void>>>
        _vectors_of_tcp_connect_socketfds_to_be_destroyed;
    std::vector<std::shared_ptr<void>>
        _current_vector_of_tcp_connect_socketfds_to_be_destroyed;
    util::Mutex _mutex_for_vectors_of_tcp_connect_socketfds_to_be_destroyed;
    std::unique_ptr<EventLoopThread>
        _tcp_connect_socketfd_destroying_thread_ptr;
#endif

    std::atomic<uint64_t> _tcp_connection_id_counter{0};

    size_t _thread_pool_capacity = 0;
    ThreadInitializationCallbackType _thread_initialization_callback;
    std::unique_ptr<EventLoopThreadPool> _thread_pool_ptr;

    uint64_t _max_number_of_connections = 0;
};

} // namespace xubinh_server

#endif