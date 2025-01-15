#include "util/condition_variable.h"

namespace xubinh_server {

namespace util {

ConditionVariable::ConditionVariable() noexcept {
    int saved_errno = ::pthread_cond_init(&_condition_variable, nullptr);

    if (saved_errno != 0) {
        ::fprintf(
            stderr,
            "@xubinh_server::util::Mutex::Mutex: pthread_mutex_init failed: %s "
            "(errno=%d)\n",
            strerror_tl(errno).c_str(),
            saved_errno
        );

        ::abort();
    }
}

ConditionVariable::~ConditionVariable() noexcept {
    int saved_errno = ::pthread_cond_destroy(&_condition_variable);

    if (saved_errno != 0) {
        ::fprintf(
            stderr,
            "@xubinh_server::util::Mutex::~Mutex: pthread_mutex_destroy "
            "failed: %s (errno=%d)\n",
            strerror_tl(errno).c_str(),
            saved_errno
        );

        ::abort();
    }
}

void ConditionVariable::notify_one() noexcept {
    int saved_errno = pthread_cond_signal(&_condition_variable);

    if (saved_errno != 0) {
        ::fprintf(
            stderr,
            "@xubinh_server::util::Mutex::notify: pthread_cond_signal failed: "
            "%s (errno=%d)\n",
            strerror_tl(errno).c_str(),
            saved_errno
        );

        ::abort();
    }
}

void ConditionVariable::notify_all() noexcept {
    int saved_errno = pthread_cond_broadcast(&_condition_variable);

    if (saved_errno != 0) {
        ::fprintf(
            stderr,
            "@xubinh_server::util::Mutex::notify_all: pthread_cond_broadcast "
            "failed: %s (errno=%d)\n",
            strerror_tl(errno).c_str(),
            saved_errno
        );

        ::abort();
    }
}

void ConditionVariable::wait(MutexGuard &mutex_guard) noexcept {
    int saved_errno = ::pthread_cond_wait(
        &_condition_variable, mutex_guard._mutex.native_handle()
    );

    if (saved_errno != 0) {
        ::fprintf(
            stderr,
            "@xubinh_server::util::Mutex::wait: pthread_cond_wait failed: %s "
            "(errno=%d)\n",
            strerror_tl(errno).c_str(),
            saved_errno
        );

        ::abort();
    }
}

ConditionVariable::TimeoutStatus ConditionVariable::wait_until(
    MutexGuard &mutex_guard, TimePoint time_point
) noexcept {
    timespec time_specification;

    time_point.to_timespec(&time_specification);

    int saved_errno = ::pthread_cond_timedwait(
        &_condition_variable,
        mutex_guard._mutex.native_handle(),
        &time_specification
    );

    if (saved_errno == 0) {
        return TimeoutStatus::NOT_TIMEOUT;
    }

    else if (saved_errno == ETIMEDOUT) {
        return TimeoutStatus::TIMEOUT;
    }

    else {
        ::fprintf(
            stderr,
            "@xubinh_server::util::Mutex::wait_until: pthread_cond_timedwait "
            "failed: %s (errno=%d)\n",
            strerror_tl(errno).c_str(),
            saved_errno
        );

        ::abort();
    }
}

} // namespace util

} // namespace xubinh_server
