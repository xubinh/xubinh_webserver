#include <cassert>

#include "util/current_thread.h"
#include "util/thread.h"

namespace xubinh_server {

namespace util {

namespace {

// 定义在未命名的命名空间中, 对外界隐藏
struct _MainThreadInitializer {
    // 只在主进程的主线程中执行一次
    _MainThreadInitializer() {
        current_thread::get_tid();

        current_thread::set_thread_name("main");

        pthread_atfork(
            nullptr,
            nullptr,
            _MainThreadInitializer::execute_in_child_after_fork
        );
    }

    // 在每个新 fork 出来的子进程的主线程中均会执行一次
    static void execute_in_child_after_fork() {
        current_thread::_tid = 0;
        current_thread::get_tid();

        current_thread::set_thread_name("main");
    }
};

// 只在主进程的主线程中初始化一次
_MainThreadInitializer _thread_name_initializer;

} // namespace

void Thread::_wrapper_of_worker_function() {
    {
        std::lock_guard<std::mutex> lock(_mutex_for_thread_info);

        _tid = current_thread::get_tid();

        // 这里字符串的生命周期等于线程对象的生命周期,
        // 因而等于线程本身的生命周期:
        current_thread::set_thread_name(_thread_name.c_str());

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

    _join_result = pthread_join(_pthread_id, nullptr);

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

int Thread::join() {
    // 确保处于已启动的状态:
    start();

    if (is_joined()) {
        return _join_result;
    }

    std::unique_lock<std::mutex> lock_for_doing_start(_mutex_for_thread_state);

    if (is_joined()) {
        return _join_result;
    }

    _do_join(std::move(lock_for_doing_start));

    return _join_result;
}

} // namespace util

} // namespace xubinh_server