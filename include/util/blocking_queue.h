#ifndef __XUBINH_SERVER_UTIL_BLOCKING_QUEUE
#define __XUBINH_SERVER_UTIL_BLOCKING_QUEUE

#include <deque>
#include <functional>

#include "util/condition_variable.h"
#include "util/mutex.h"

namespace xubinh_server {

namespace util {

// fixed size with no timeout
template <typename ManagedType>
class BlockingQueue {
public:
#ifdef __USE_BLOCKING_QUEUE_WITH_RAW_POINTER
    using ElementType = ManagedType *;
#else
    using ElementType = ManagedType;
#endif
    using ContainerType = std::deque<ElementType>;

    explicit BlockingQueue(int capacity) : _capacity(capacity) {
    }

    // no copy
    BlockingQueue(const BlockingQueue &) = delete;
    BlockingQueue &operator=(const BlockingQueue &) = delete;

    // no move
    BlockingQueue(BlockingQueue &&) = delete;
    BlockingQueue &operator=(BlockingQueue &&) = delete;

    ~BlockingQueue();

    void push(ManagedType element);

    ElementType pop();

    ContainerType pop_all();

private:
    ContainerType _queue;
    const size_t _capacity;
    Mutex _mutex;
    ConditionVariable _cond_queue_not_full;
    ConditionVariable _cond_queue_not_empty;
};

// declaration
extern template class BlockingQueue<std::function<void()>>;

} // namespace util

} // namespace xubinh_server

#endif