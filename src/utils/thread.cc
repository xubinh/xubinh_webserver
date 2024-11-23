#include <cassert>

#include "../../include/utils/thread.h"

namespace xubinh_server {

namespace utils {

void Thread::_wrapper_of_worker_function() {
    {
        std::lock_guard<std::mutex> lock(_mutex_for_thread_info);

        _tid = Thread::_get_tid();

        _cond_for_thread_info.notify_all();
    }

    _worker_function();
}

void Thread::_do_start(std::unique_lock<std::mutex> lock) {
    assert(!is_started());

    std::unique_lock<std::mutex> lock_for_initializing_state(
        _mutex_for_thread_info
    );

    if (pthread_create(
            &_pthread_id,
            nullptr,
            Thread::_adaptor_function_for_pthread_create,
            this
        )) {

        /* [TODO]: make some log first before aborting */

        abort();
    }

    _is_started = 1;

    _cond_for_thread_info.wait(lock_for_initializing_state);
}

void Thread::_do_join(std::unique_lock<std::mutex> lock) {
    assert(!is_joined());

    pthread_join(_pthread_id, nullptr);

    _is_joined = 1;
}

void *Thread::_adaptor_function_for_pthread_create(void *arg) {
    Thread *thread = reinterpret_cast<Thread *>(arg);

    thread->_wrapper_of_worker_function();

    return nullptr;
}

Thread::Thread(
    WorkerFunctionType worker_function, const std::string &thread_name
)
    : _worker_function(std::move(worker_function)), _thread_name(thread_name) {
}

void Thread::start() {
    if (is_started()) {
        return;
    }

    std::unique_lock<std::mutex> lock_for_doing_start(_mutex_for_thread_state);

    if (is_started()) {
        return;
    }

    _do_start(std::move(lock_for_doing_start));

    return;
}

pid_t Thread::get_tid() {
    if (is_started()) {
        return _tid;
    }

    std::unique_lock<std::mutex> lock_for_doing_start(_mutex_for_thread_state);

    if (is_started()) {
        return _tid;
    }

    _do_start(std::move(lock_for_doing_start));

    return _tid;
}

void Thread::join() {
    // 确保处于已启动的状态:
    start();

    if (is_joined()) {
        return;
    }

    std::unique_lock<std::mutex> lock_for_doing_start(_mutex_for_thread_state);

    if (is_joined()) {
        return;
    }

    _do_join(std::move(lock_for_doing_start));

    return;
}

} // namespace utils

} // namespace xubinh_server