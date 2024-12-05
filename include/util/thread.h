#ifndef XUBINH_SERVER_UTIL_THREAD
#define XUBINH_SERVER_UTIL_THREAD

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <unistd.h>

namespace xubinh_server {

namespace util {

// Simple wrapper for POSIX thread
//
// - Only be responsible for executing a functor passed in in a separate thread.
// For the thread to be joined successfully later, user must first set up
// appropriate external flags (i.e., outside this `Thread` object)
// indicating the end of execution.
class Thread {
public:
    using WorkerFunctionType = std::function<void()>;

    Thread(WorkerFunctionType worker_function, const std::string &thread_name)
        : _worker_function(std::move(worker_function)),
          _thread_name(thread_name) {
    }

    // no copy
    Thread(const Thread &) = delete;
    Thread &operator=(const Thread &) = delete;

    // no move
    Thread(Thread &&) = delete;
    Thread &operator=(Thread &&) = delete;

    ~Thread();

    bool is_started() const {
        return _is_started;
    }

    bool is_joined() const {
        return _is_joined;
    }

    void start();

    pid_t get_tid();

    // this method must be called after setting up appropriate external flags
    void join();

private:
    static void *_adaptor_function_for_pthread_create(void *arg);

    void _wrapper_of_worker_function();

    void _do_start(std::unique_lock<std::mutex>);

    void _do_join(std::unique_lock<std::mutex>);

    bool _is_started = false;
    bool _is_joined = false;
    pthread_t _pthread_id = std::numeric_limits<pthread_t>::max();
    pid_t _tid = -1;
    WorkerFunctionType _worker_function;
    const std::string _thread_name;

    // external mutex for methods like `start`, `get_pid`, `join`, etc.
    std::mutex _external_mutex;

    // internal mutex and cond for thread info like TID, thread name, etc.
    std::mutex _internal_mutex;
    std::condition_variable _internal_cond;
};

} // namespace util

} // namespace xubinh_server

#endif