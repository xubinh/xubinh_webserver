#ifndef __XUBINH_SERVER_EVENT_LOOP_THREAD_POOL
#define __XUBINH_SERVER_EVENT_LOOP_THREAD_POOL

#include <memory>
#include <vector>

#include "event_loop.h"
#include "event_loop_thread.h"

namespace xubinh_server {

// not thread-safe
class EventLoopThreadPool {
public:
    using ThreadInitializationCallbackType =
        EventLoopThread::ThreadInitializationCallbackType;

    EventLoopThreadPool(size_t thread_pool_capasity);

    ~EventLoopThreadPool();

    void register_thread_initialization_callback(
        ThreadInitializationCallbackType thread_initialization_callback
    ) {
        _thread_initialization_callback =
            std::move(thread_initialization_callback);
    }

    // start all the worker threads
    void start();

    // stop all the worker threads
    void stop();

    // poll for whether all the worker threads are joinable
    bool is_joinable() const {
        for (auto &thread_ptr : _thread_pool) {
            if (!thread_ptr->is_joinable()) {
                return false;
            }
        }

        return true;
    }

    // join all the worker threads
    void join();

    EventLoop *get_next_loop();

private:
    const size_t _thread_pool_capasity;

    ThreadInitializationCallbackType _thread_initialization_callback;

    bool _is_started = false;
    bool _is_stopped = false;
    bool _is_joined = false;

    std::vector<std::shared_ptr<EventLoopThread>> _thread_pool;
    std::atomic<int> _next_loop_index_counter{0};
};

} // namespace xubinh_server

#endif