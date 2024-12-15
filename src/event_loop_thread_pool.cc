#include "event_loop_thread_pool.h"

namespace xubinh_server {

EventLoopThreadPool::EventLoopThreadPool(
    size_t capacity,
    ThreadInitializationCallbackType thread_initialization_callback
)
    : _THREAD_POOL_CAPASITY(capacity) {
    if (capacity == 0) {
        LOG_FATAL << "zero capacity given for initializing thread pool";
    }

    for (int i = 0; i < capacity; i++) {
        _thread_pool.push_back(std::make_shared<EventLoopThread>(
            "#" + std::to_string(i) + "@EventLoopThreadPool",
            thread_initialization_callback
        ));
    }
}

EventLoopThreadPool::~EventLoopThreadPool() {
    if (!_is_stopped) {
        LOG_FATAL << "tried to destruct a thread pool before stopping it";
    }
}

void EventLoopThreadPool::start() {
    if (_is_started) {
        return;
    }

    for (auto &thread : _thread_pool) {
        thread->start();
    }

    _is_started = true;
}

void EventLoopThreadPool::stop() {
    if (_is_stopped) {
        return;
    }

    if (!_is_started) {
        LOG_FATAL << "tried to stop a thread pool before starting it";
    }

    for (auto &thread : _thread_pool) {
        thread->get_loop()->ask_to_stop();
    }

    for (auto &thread : _thread_pool) {
        thread->join();
    }

    _is_stopped = true;
}

EventLoop *EventLoopThreadPool::get_next_loop() {
    auto next_loop_index =
        _next_loop_index_counter.fetch_add(1, std::memory_order_relaxed)
        % _THREAD_POOL_CAPASITY;

    return _thread_pool[next_loop_index]->get_loop();
}

} // namespace xubinh_server