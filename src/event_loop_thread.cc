#include "event_loop_thread.h"

namespace xubinh_server {

explicit EventLoopThread::EventLoopThread(
    const std::string &thread_name,
    ThreadInitializationCallbackType thread_initialization_callback,
    uint64_t loop_index = 0
)
    : _thread(
        [this, loop_index]() {
            _worker_function(loop_index);
        },
        thread_name
    ),
      _thread_initialization_callback(std::move(thread_initialization_callback)
      ) {
}

EventLoopThread::~EventLoopThread() {
    if (!is_joined()) {
        LOG_FATAL << "event loop thread must be joined before destruction";
    }
}

void EventLoopThread::start() {
    {
        std::unique_lock<std::mutex> lock(_mutex);

        _thread.start();

        _cond.wait(lock, [this]() {
            return _loop_ptr != nullptr;
        });
    }
}

void EventLoopThread::_worker_function(uint64_t loop_index) {
    EventLoop loop(loop_index);

    {
        std::lock_guard<std::mutex> lock(_mutex);

        _loop_ptr = &loop;
    }

    _cond.notify_all();

    if (_thread_initialization_callback) {
        _thread_initialization_callback();
    }

    loop.loop();
}

} // namespace xubinh_server