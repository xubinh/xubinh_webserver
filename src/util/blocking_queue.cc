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
            })
        }

        _queue.push_back(std::move(element)); // move whenever possible
    }

    _cond_queue_not_empty.notify_one();
}

template <typename T>
T BlockingQueue<T>::pop() {
    {
        std::unique_lock<std::mutex> lock(_mutex);

        if (_queue.empty()) {
            // [TODO]: add timeout
            _cond_queue_not_empty.wait(lock, [this]() {
                return !_queue.empty();
            })
        }

        auto popped_element = _queue.pop_front(); // RVO, no need to move
    }

    _cond_queue_not_full.notify_one();

    return popped_element; // RVO, no need to move
}

} // namespace util

} // namespace xubinh_server