#ifndef XUBINH_SERVER_EVENT_LOOP_THREAD_POOL
#define XUBINH_SERVER_EVENT_LOOP_THREAD_POOL

#include <vector>

#include "event_loop.h"
#include "event_loop_thread.h"
#include "log_builder.h"

namespace xubinh_server {

// not thread-safe
class EventLoopThreadPool {
public:
    using ThreadInitializationCallbackType =
        EventLoopThread::ThreadInitializationCallbackType;

    EventLoopThreadPool(
        size_t capacity,
        ThreadInitializationCallbackType thread_initialization_callback
    );

    ~EventLoopThreadPool();

    void start();

    void stop();

    EventLoopThread *get_next_thread();

    EventLoop *get_next_loop() {
        return get_next_thread()->get_loop();
    }

private:
    const size_t _THREAD_POOL_CAPASITY = 0;

    bool _is_started = false;
    bool _is_stopped = false;

    std::vector<EventLoopThread> _thread_pool;
    std::atomic<int> _next_loop_index_counter{0};
};

} // namespace xubinh_server

#endif