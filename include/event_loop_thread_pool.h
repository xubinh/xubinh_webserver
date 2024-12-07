#ifndef XUBINH_SERVER_EVENT_LOOP_THREAD_POOL
#define XUBINH_SERVER_EVENT_LOOP_THREAD_POOL

#include <vector>

#include "event_loop.h"
#include "event_loop_thread.h"

namespace xubinh_server {

class EventLoopThreadPool {
public:
    explicit EventLoopThreadPool(int capacity)
        : _THREAD_POOL_CAPASITY(capacity), _thread_pool(capacity) {
        for (auto &thread : _thread_pool) {
            thread.start();
        }
    }

    ~EventLoopThreadPool() {
        for (auto &thread : _thread_pool) {
            thread.get_loop()->ask_to_stop();
        }

        for (auto &thread : _thread_pool) {
            thread.join();
        }
    }

    EventLoopThread *get_next_thread() {
        auto next_loop_index =
            _next_loop_index_counter.fetch_add(1, std::memory_order_acquire)
            % _THREAD_POOL_CAPASITY;

        return &_thread_pool[next_loop_index];
    }

private:
    const int _THREAD_POOL_CAPASITY = -1;

    std::vector<EventLoopThread> _thread_pool;
    std::atomic<int> _next_loop_index_counter{0};
};

} // namespace xubinh_server

#endif