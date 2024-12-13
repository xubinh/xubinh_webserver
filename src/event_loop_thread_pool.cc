#include "event_loop_thread_pool.h"

namespace xubinh_server {

explicit EventLoopThreadPool::EventLoopThreadPool(
    int capacity,
    ThreadInitializationCallbackType thread_initialization_callback
)
    : _THREAD_POOL_CAPASITY(capacity) {
    if (capacity <= 0) {
        LOG_FATAL << "non-positive thread pool capacity given";
    }

    for (int i = 0; i < capacity; ++i) {
        _thread_pool.emplace_back(
            "#" + std::to_string(i) + "@EventLoopThreadPool",
            thread_initialization_callback
        );
    }
}

EventLoopThreadPool::~EventLoopThreadPool() {
    for (auto &thread : _thread_pool) {
        if (thread.is_joined()) {
            continue;
        }

        LOG_FATAL << "event loop thread pool must be stopped before "
                     "destruction";
    }
}

void EventLoopThreadPool::start() {
    for (auto &thread : _thread_pool) {
        thread.start();
    }
}

void EventLoopThreadPool::stop() {
    for (auto &thread : _thread_pool) {
        thread.get_loop()->ask_to_stop();
    }

    for (auto &thread : _thread_pool) {
        thread.join();
    }
}

EventLoopThread *EventLoopThreadPool::get_next_thread() {
    auto next_loop_index =
        _next_loop_index_counter.fetch_add(1, std::memory_order_acquire)
        % _THREAD_POOL_CAPASITY;

    return &_thread_pool[next_loop_index];
}

} // namespace xubinh_server