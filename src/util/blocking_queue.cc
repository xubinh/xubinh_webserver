#include <functional>

#include "util/blocking_queue.h"

namespace xubinh_server {

namespace util {

#ifdef __USE_BLOCKING_QUEUE_WITH_RAW_POINTER
template <typename T>
BlockingQueue<T>::~BlockingQueue() {
    {
        std::lock_guard<std::mutex> lock(_mutex);

        while (!_queue.empty()) {
            delete _queue.front();

            _queue.pop_front();
        }
    }
}
#endif

template <typename T>
void BlockingQueue<T>::push(T element) {
    {
        std::unique_lock<std::mutex> lock(_mutex);

        if (_queue.size() >= _capacity) {
            // [TODO]: add timeout
            _cond_queue_not_full.wait(lock, [this]() {
                return _queue.size() < _capacity;
            });
        }

#ifdef __USE_BLOCKING_QUEUE_WITH_RAW_POINTER
        _queue.push_back(new T(std::move(element)));
#else
        _queue.push_back(std::move(element));
#endif
    }

    _cond_queue_not_empty.notify_one();
}

template <typename T>
#ifdef __USE_BLOCKING_QUEUE_WITH_RAW_POINTER
T *BlockingQueue<T>::pop() {
    T *popped_element;
#else
T BlockingQueue<T>::pop() {
    T popped_element;
#endif

    {
        std::unique_lock<std::mutex> lock(_mutex);

        if (_queue.empty()) {
            // [TODO]: add timeout
            _cond_queue_not_empty.wait(lock, [this]() {
                return !_queue.empty();
            });
        }

#ifdef __USE_BLOCKING_QUEUE_WITH_RAW_POINTER
        popped_element = _queue.front();
#else
        popped_element = std::move(_queue.front());
#endif

        _queue.pop_front();
    }

    _cond_queue_not_full.notify_one();

    return popped_element;
}

template <typename T>
typename BlockingQueue<T>::ContainerType BlockingQueue<T>::pop_all() {
    ContainerType queue;

    {
        std::lock_guard<std::mutex> lock(_mutex);

        queue.swap(_queue);
    }

    _cond_queue_not_full.notify_all();

    return queue;
}

// explicit instantiation
template class BlockingQueue<std::function<void()>>;

} // namespace util

} // namespace xubinh_server