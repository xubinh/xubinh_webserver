#ifndef XUBINH_SERVER_UTIL_BLOCKING_QUEUE
#define XUBINH_SERVER_UTIL_BLOCKING_QUEUE

#include <condition_variable>
#include <deque>
#include <mutex>
#include <stdexcept>

namespace xubinh_server {

namespace util {

// fixed size with no timeout (be careful)
template <typename T>
class BlockingQueue {
public:
    // no default constructor
    BlockingQueue() = delete;

    // must specify a capacity explicitly
    explicit BlockingQueue(const int capacity) : _capacity(capacity) {
    }

    // no copying
    BlockingQueue(const BlockingQueue &) = delete;
    BlockingQueue &operator=(const BlockingQueue &) = delete;

    // no moving
    BlockingQueue(BlockingQueue &&) = delete;
    BlockingQueue &operator=(BlockingQueue &&) = delete;

    void push(T element);

    T pop();

private:
    std::deque<T> _queue;
    const size_t _capacity;
    std::mutex _mutex;
    std::condition_variable _cond_queue_not_full;
    std::condition_variable _cond_queue_not_empty;
};

} // namespace util

} // namespace xubinh_server

#endif