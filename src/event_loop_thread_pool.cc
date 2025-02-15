#include "event_loop_thread_pool.h"
#include "log_builder.h"

namespace xubinh_server {

EventLoopThreadPool::EventLoopThreadPool(size_t thread_pool_capasity)
    : _thread_pool_capasity(thread_pool_capasity) {
    if (thread_pool_capasity == 0) {
        LOG_FATAL << "zero capacity given for initializing thread pool";
    }
}

EventLoopThreadPool::~EventLoopThreadPool() {
    if (!_is_stopped) {
        LOG_FATAL << "tried to destruct a thread pool before stopping it";
    }

    if (!_is_joined) {
        LOG_WARN
            << "thread pool destruction before joining all the worker threads";

        join();
    }

    LOG_INFO << "exit destructor: EventLoopThreadPool";
}

void EventLoopThreadPool::start() {
    if (_is_started) {
        return;
    }

    for (int i = 0; i < static_cast<int>(_thread_pool_capasity); i++) {
        std::string thread_name = "worker-thread-" + std::to_string(i);

        size_t loop_index = i;

        _thread_pool.push_back(std::make_shared<EventLoopThread>(
            thread_name, _thread_initialization_callback, loop_index
        ));

        _thread_pool.back()->start();
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

    for (auto &thread_ptr : _thread_pool) {
        thread_ptr->get_loop()->ask_to_stop();
    }

    _is_stopped = true;
}

void EventLoopThreadPool::join() {
    if (_is_joined) {
        LOG_WARN << "double join";

        return;
    }

    LOG_TRACE << "joining threads...";

    for (auto &thread_ptr : _thread_pool) {
        thread_ptr->join();
    }

    _is_joined = true;

    LOG_TRACE << "all threads are joined";
}

EventLoop *EventLoopThreadPool::get_next_loop() {
    auto next_loop_index =
        _next_loop_index_counter.fetch_add(1, std::memory_order_relaxed)
        % _thread_pool_capasity;

    return _thread_pool[next_loop_index]->get_loop();
}

} // namespace xubinh_server