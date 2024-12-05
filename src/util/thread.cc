#include <cassert>

#include "log_builder.h"
#include "util/current_thread.h"
#include "util/thread.h"

namespace xubinh_server {

namespace util {

namespace {

// initializar for main thread's TID and thread name
struct _MainThreadInitializer {
    // make sure child process's main thread is initialized, too
    static void execute_in_child_after_fork() {
        current_thread::reset_tid();
        current_thread::get_tid();

        current_thread::set_thread_name("main");
    }

    // put initialization code inside the constructor of a static life-time
    // object for it to be executed before anything starts and to be executed
    // only once
    _MainThreadInitializer() {
        current_thread::get_tid();

        current_thread::set_thread_name("main");

        pthread_atfork(
            nullptr,
            nullptr,
            _MainThreadInitializer::execute_in_child_after_fork
        );
    }
};

// define a static life-time object
_MainThreadInitializer _thread_name_initializer;

} // namespace

Thread::~Thread() {
    {
        std::unique_lock<std::mutex> external_lock(_external_mutex);

        if (!is_joined()) {
            LOG_FATAL << "destructor called before joining of the thread";
        }
    }
}

void Thread::start() {
    {
        std::unique_lock<std::mutex> external_lock(_external_mutex);

        if (!is_started()) {
            _do_start(std::move(external_lock));
        }
    }

    return;
}

pid_t Thread::get_tid() {
    start();

    return _tid;
}

void Thread::join() {
    // just in case, appropriate external flags are still required
    start();

    {
        std::unique_lock<std::mutex> external_lock(_external_mutex);

        if (!is_joined()) {
            _do_join(std::move(external_lock));
        }
    }

    return;
}

void *Thread::_adaptor_function_for_pthread_create(void *arg) {
    Thread *thread = reinterpret_cast<Thread *>(arg);

    thread->_wrapper_of_worker_function();

    return nullptr;
}

void Thread::_wrapper_of_worker_function() {
    {
        std::lock_guard<std::mutex> lock(_internal_mutex);

        _tid = current_thread::get_tid();

        current_thread::set_thread_name(_thread_name.c_str());
    }

    _internal_cond.notify_all();

    _worker_function();
}

void Thread::_do_start(std::unique_lock<std::mutex>) {
    {
        std::unique_lock<std::mutex> lock_for_thread_info(_internal_mutex);

        if (pthread_create(
                &_pthread_id,
                nullptr,
                Thread::_adaptor_function_for_pthread_create,
                this
            )) {

            LOG_SYS_FATAL << "Failed to create new thread";
        }

        _is_started = 1;

        // handover control to the background thread
        _internal_cond.wait(lock_for_thread_info, [this]() {
            return _tid > 0;
        });
    }
}

void Thread::_do_join(std::unique_lock<std::mutex>) {
    // the thread itself never returns a result
    auto _pthread_join_result = pthread_join(_pthread_id, nullptr);

    if (_pthread_join_result) {
        switch (errno) {
        case EDEADLK: {
            LOG_FATAL << "dead lock detected when joining a thread";
        }

        // will never encounter these, so make them fatal
        case EINVAL:
        case ESRCH: {
            LOG_FATAL << "something that is not supposed to happen happened";
        }

        default: {
            LOG_SYS_ERROR << "unknown error";
        }
        }
    }

    _is_joined = 1;
}

} // namespace util

} // namespace xubinh_server