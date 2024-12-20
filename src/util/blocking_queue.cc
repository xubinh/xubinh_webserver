#include <functional>

#include "util/blocking_queue.h"

namespace xubinh_server {

namespace util {

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

        _queue.push_back(std::move(element)); // move whenever possible
    }

    _cond_queue_not_empty.notify_one();
}

template <typename T>
T BlockingQueue<T>::pop() {
    T popped_element;

    {
        std::unique_lock<std::mutex> lock(_mutex);

        if (_queue.empty()) {
            // [TODO]: add timeout
            _cond_queue_not_empty.wait(lock, [this]() {
                return !_queue.empty();
            });
        }

        popped_element = std::move(_queue.front());

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