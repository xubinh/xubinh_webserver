#ifndef XUBINH_SERVER_EVENT_LOOP_THREAD
#define XUBINH_SERVER_EVENT_LOOP_THREAD

#include <condition_variable>
#include <mutex>

#include "event_loop.h"
#include "util/thread.h"

namespace xubinh_server {

// not thread-safe
class EventLoopThread {
private:
    using Thread = util::Thread;

public:
    explicit EventLoopThread(const std::string &thread_name)
        : _thread(std::bind(_worker_function, this), thread_name) {
    }

    // no copy
    EventLoopThread(const EventLoopThread &) = delete;
    EventLoopThread &operator=(const EventLoopThread &) = delete;

    // no move
    EventLoopThread(EventLoopThread &&) = delete;
    EventLoopThread &operator=(EventLoopThread &&) = delete;

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
    void _worker_function();

    Thread _thread;

    EventLoop *_loop_ptr = nullptr;
    std::mutex _mutex;
    std::condition_variable _cond;
};

} // namespace xubinh_server

#endif