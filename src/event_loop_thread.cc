#include "event_loop_thread.h"
#include "log_builder.h"
#include "util/mutex_guard.h"

namespace xubinh_server {

EventLoopThread::EventLoopThread(
    const std::string &thread_name,
    ThreadInitializationCallbackType thread_initialization_callback,
    uint64_t loop_index
)
    : _thread_initialization_callback(std::move(thread_initialization_callback))
    , _thread(
          [this, loop_index]() {
              _worker_function(loop_index);
          },
          thread_name
      ) {
}

EventLoopThread::~EventLoopThread() {
    if (!is_joined()) {
        LOG_FATAL << "event loop thread must be joined before destruction";
    }
}

void EventLoopThread::start() {
    {
        util::MutexGuard lock(_mutex);

        _thread.start();

        _cond.wait(lock, [this]() {
            return _loop_ptr != nullptr;
        });
    }
}

void EventLoopThread::_worker_function(uint64_t loop_index) {
    EventLoop loop(loop_index);

    {
        util::MutexGuard lock(_mutex);

        _loop_ptr = &loop;
    }

    _cond.notify_all();

    if (_thread_initialization_callback) {
        _thread_initialization_callback();
    }

    loop.loop();
}

} // namespace xubinh_server