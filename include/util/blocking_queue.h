#ifndef __XUBINH_SERVER_UTIL_BLOCKING_QUEUE
#define __XUBINH_SERVER_UTIL_BLOCKING_QUEUE

#include <condition_variable>
#include <deque>
#include <mutex>
#include <stdexcept>

namespace xubinh_server {

namespace util {

// fixed size with no timeout
template <typename T>
class BlockingQueue {
public:
    using ContainerType = std::deque<T>;

    explicit BlockingQueue(int capacity) : _capacity(capacity) {
    }

    // no copy
    BlockingQueue(const BlockingQueue &) = delete;
    BlockingQueue &operator=(const BlockingQueue &) = delete;

    // no move
    BlockingQueue(BlockingQueue &&) = delete;
    BlockingQueue &operator=(BlockingQueue &&) = delete;

    void push(T element);

    T pop();

    ContainerType pop_all();

private:
    std::deque<T> _queue;
    const size_t _capacity;
    std::mutex _mutex;
    std::condition_variable _cond_queue_not_full;
    std::condition_variable _cond_queue_not_empty;
};

// declaration
extern template class BlockingQueue<std::function<void()>>;

} // namespace util

} // namespace xubinh_server

#endif