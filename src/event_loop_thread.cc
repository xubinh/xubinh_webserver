#include "event_loop_thread.h"

namespace xubinh_server {

void EventLoopThread::start() {
    {
        std::unique_lock<std::mutex> lock(_mutex);

        _thread.start();

        _cond.wait(lock, [this]() {
            return _loop_ptr != nullptr;
        });
    }
}

void EventLoopThread::_worker_function() {
    EventLoop loop;

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