#include "util/thread.h"
#include "util/this_thread.h"

namespace xubinh_server {

namespace util {

namespace {

// static initializar for main thread's TID and thread name
struct MainThreadInitializer {
    MainThreadInitializer() {
        this_thread::get_tid();

        this_thread::set_thread_name("main-thread");
    }
} main_thread_initializer;

} // namespace

Thread::~Thread() {
    if (!_is_joined) {
        ::fprintf(
            stderr,
            "@xubinh_server::util::Thread::~Thread: destructor called before "
            "joining of the thread\n"
        );

        ::abort();
    }
}

void Thread::start() {
    if (_is_started) {
        return;
    }

    {
        util::MutexGuard lock(_mutex);

        if (::pthread_create(
                &_pthread_id,
                nullptr,
                Thread::_adaptor_function_for_pthread_create,
                this
            )) {

            ::fprintf(
                stderr,
                "@xubinh_server::util::Thread::start: failed to create new "
                "thread\n"
            );

            ::abort();
        }

        // handover control to the worker thread
        _cond.wait(lock, [this]() {
            return _tid > 0;
        });
    }

    _is_started = true;
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
            ::fprintf(
                stderr,
                "@xubinh_server::util::Thread::join: dead lock detected when "
                "joining a thread\n"
            );

            ::abort();

            break;

        // will never encounter these, so make them fatal
        case EINVAL:
        case ESRCH:
            ::fprintf(
                stderr,
                "@xubinh_server::util::Thread::join: something that is not "
                "supposed to happen happened\n"
            );

            ::abort();

            break;

        default:
            ::fprintf(
                stderr, "@xubinh_server::util::Thread::join: unknown error\n"
            );

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
        util::MutexGuard lock(_mutex);

        _tid = this_thread::get_tid();

        this_thread::set_thread_name(_thread_name.c_str());
    }

    _cond.notify_all();

    _worker_function();
}

} // namespace util

} // namespace xubinh_server