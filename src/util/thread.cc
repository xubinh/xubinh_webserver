#include "util/thread.h"
#include "util/current_thread.h"

namespace xubinh_server {

namespace util {

namespace {

// initializar for main thread's TID and thread name
struct _MainThreadInitializer {
    // make sure child process's main thread is initialized, too
    static void execute_in_child_after_fork() {
        current_thread::reset_tid();
        current_thread::get_tid();

        current_thread::set_thread_name(MAIN_THREAD_NAME);
    }

    // put initialization code inside the constructor of a static life-time
    // object for it to be executed before anything starts and to be executed
    // only once
    _MainThreadInitializer() {
        current_thread::get_tid();

        current_thread::set_thread_name(MAIN_THREAD_NAME);

        pthread_atfork(
            nullptr,
            nullptr,
            _MainThreadInitializer::execute_in_child_after_fork
        );
    }

    static const char *MAIN_THREAD_NAME;
};

const char *_MainThreadInitializer::MAIN_THREAD_NAME = "main-thread";

// define a static life-time object
_MainThreadInitializer _thread_name_initializer;

} // namespace

Thread::~Thread() {
    if (!_is_joined) {
        fprintf(stderr, "destructor called before joining of the thread\n");

        ::abort();
    }
}

void Thread::start() {
    if (_is_started) {
        return;
    }

    {
        std::unique_lock<std::mutex> lock(_mutex);

        if (::pthread_create(
                &_pthread_id,
                nullptr,
                Thread::_adaptor_function_for_pthread_create,
                this
            )) {

            ::fprintf(stderr, "failed to create new thread\n");

            ::abort();
        }

        // handover control to the worker thread
        _cond.wait(lock, [this]() {
            return _tid > 0;
        });
    }

    _is_started = true;

    // // debug
    // fprintf(
    //     stderr,
    //     "thread started, name: %s, TID: %d\n",
    //     _thread_name.c_str(),
    //     _tid
    // );
}

void Thread::join() {
    // just in case, appropriate external flags are still required
    start();

    if (_is_joined) {
        return;
    }

    // the thread itself never returns a result
    auto _pthread_join_result = pthread_join(_pthread_id, nullptr);

    if (_pthread_join_result) {
        switch (errno) {
        case EDEADLK:
            fprintf(stderr, "dead lock detected when joining a thread\n");

            ::abort();

            break;

        // will never encounter these, so make them fatal
        case EINVAL:
        case ESRCH:
            fprintf(
                stderr, "something that is not supposed to happen happened\n"
            );

            ::abort();

            break;

        default:
            fprintf(stderr, "unknown error\n");

            break;
        }
    }

    _is_joined = true;
}

void *Thread::_adaptor_function_for_pthread_create(void *arg) {
    Thread *thread = reinterpret_cast<Thread *>(arg);

    thread->_wrapper_of_worker_function();

    return nullptr;
}

void Thread::_wrapper_of_worker_function() {
    {
        std::lock_guard<std::mutex> lock(_mutex);

        _tid = current_thread::get_tid();

        current_thread::set_thread_name(_thread_name.c_str());
    }

    _cond.notify_all();

    _worker_function();
}

} // namespace util

} // namespace xubinh_server