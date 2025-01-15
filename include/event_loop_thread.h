#ifndef __XUBINH_SERVER_EVENT_LOOP_THREAD
#define __XUBINH_SERVER_EVENT_LOOP_THREAD

#include "event_loop.h"
#include "util/condition_variable.h"
#include "util/mutex.h"
#include "util/thread.h"

namespace xubinh_server {

// not thread-safe
class EventLoopThread {
private:
    using Thread = util::Thread;

public:
    using ThreadInitializationCallbackType = std::function<void()>;

    explicit EventLoopThread(
        const std::string &thread_name,
        ThreadInitializationCallbackType thread_initialization_callback,
        uint64_t loop_index = 0
    );

    // no copy
    EventLoopThread(const EventLoopThread &) = delete;
    EventLoopThread &operator=(const EventLoopThread &) = delete;

    // no move
    EventLoopThread(EventLoopThread &&) = delete;
    EventLoopThread &operator=(EventLoopThread &&) = delete;

    ~EventLoopThread();

    bool is_started() const {
        return _thread.is_started();
    }

    bool is_joined() const {
        return _thread.is_joined();
    }

    void start();

    EventLoop *get_loop() const {
        return _loop_ptr;
    }

    pid_t get_tid() {
        return _thread.get_tid();
    }

    void join() {
        _thread.join();
    }

private:
    void _worker_function(uint64_t loop_index);

    ThreadInitializationCallbackType _thread_initialization_callback;

    Thread _thread;

    EventLoop *_loop_ptr = nullptr;
    util::Mutex _mutex;
    util::ConditionVariable _cond;
};

} // namespace xubinh_server

#endif