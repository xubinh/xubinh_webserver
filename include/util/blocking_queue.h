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
#ifdef __USE_BLOCKING_QUEUE_WITH_RAW_POINTER
    using ContainerType = std::deque<T *>;
#else
    using ContainerType = std::deque<T>;
#endif

    explicit BlockingQueue(int capacity) : _capacity(capacity) {
    }

    // no copy
    BlockingQueue(const BlockingQueue &) = delete;
    BlockingQueue &operator=(const BlockingQueue &) = delete;

    // no move
    BlockingQueue(BlockingQueue &&) = delete;
    BlockingQueue &operator=(BlockingQueue &&) = delete;

#ifdef __USE_BLOCKING_QUEUE_WITH_RAW_POINTER
    ~BlockingQueue();
#else
    ~BlockingQueue() = default;
#endif

    void push(T element);

#ifdef __USE_BLOCKING_QUEUE_WITH_RAW_POINTER
    T *pop();
#else
    T pop();
#endif

    ContainerType pop_all();

private:
    ContainerType _queue;
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