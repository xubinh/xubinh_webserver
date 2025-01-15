#ifndef __XUBINH_SERVER_UTIL_MUTEX_GUARD
#define __XUBINH_SERVER_UTIL_MUTEX_GUARD

#include "util/mutex.h"

namespace xubinh_server {

namespace util {

class MutexGuard {
public:
    explicit MutexGuard(Mutex &mutex) noexcept : _mutex(mutex) {
        _mutex.lock();
    }

    MutexGuard(const MutexGuard &) = delete;
    MutexGuard &operator=(const MutexGuard &) = delete;

    MutexGuard(MutexGuard &&) = delete;
    MutexGuard &operator=(MutexGuard &&) = delete;

    ~MutexGuard() noexcept {
        _mutex.unlock();
    }

private:
    friend class ConditionVariable;

    Mutex &_mutex;
};

} // namespace util

} // namespace xubinh_server

#endif
