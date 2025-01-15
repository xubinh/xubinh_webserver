#ifndef __XUBINH_SERVER_UTIL_CONDITION_VARIABLE
#define __XUBINH_SERVER_UTIL_CONDITION_VARIABLE

#include <pthread.h>

#include "util/errno.h"
#include "util/mutex_guard.h"
#include "util/time_point.h"

namespace xubinh_server {

namespace util {

class ConditionVariable {
public:
    enum class TimeoutStatus { NOT_TIMEOUT, TIMEOUT };

    ConditionVariable() noexcept;

    ConditionVariable(const ConditionVariable &) = delete;
    ConditionVariable &operator=(const ConditionVariable &) = delete;

    ConditionVariable(ConditionVariable &&) = delete;
    ConditionVariable &operator=(ConditionVariable &&) = delete;

    ~ConditionVariable() noexcept;

    void notify_one() noexcept;

    void notify_all() noexcept;

    void wait(MutexGuard &mutex_guard) noexcept;

    template <typename PredicateType>
    void wait(MutexGuard &mutex_guard, PredicateType predicate) noexcept {
        while (!predicate()) {
            wait(mutex_guard);
        }
    }

    TimeoutStatus
    wait_until(MutexGuard &mutex_guard, TimePoint time_point) noexcept;

    template <typename PredicateType>
    bool wait_until(
        MutexGuard &mutex_guard, TimePoint time_point, PredicateType predicate
    ) noexcept {
        while (!predicate()) {
            if (wait_until(mutex_guard, time_point) == TimeoutStatus::TIMEOUT) {
                return predicate();
            }
        }

        return true;
    }

    TimeoutStatus
    wait_for(MutexGuard &mutex_guard, TimeInterval time_interval) noexcept {
        auto time_point = TimePoint() + time_interval;

        return wait_until(mutex_guard, time_point);
    }

    template <typename PredicateType>
    bool wait_for(
        MutexGuard &mutex_guard,
        TimeInterval time_interval,
        PredicateType predicate
    ) noexcept {
        auto time_point = TimePoint() + time_interval;

        return wait_until(mutex_guard, time_point, std::move(predicate));
    }

private:
    pthread_cond_t _condition_variable;
};

} // namespace util

} // namespace xubinh_server

#endif
