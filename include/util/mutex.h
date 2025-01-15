#ifndef __XUBINH_SERVER_UTIL_MUTEX
#define __XUBINH_SERVER_UTIL_MUTEX

#include <pthread.h>

namespace xubinh_server {

namespace util {

class Mutex {
public:
    Mutex() noexcept;

    Mutex(const Mutex &) = delete;
    Mutex &operator=(const Mutex &) = delete;

    Mutex(Mutex &&) = delete;
    Mutex &operator=(Mutex &&) = delete;

    ~Mutex() noexcept;

    void lock() noexcept;

    bool try_lock() noexcept;

    void unlock() noexcept;

    pthread_mutex_t *native_handle() noexcept {
        return &_pthread_mutex;
    }

private:
    pthread_mutex_t _pthread_mutex;
};

} // namespace util

} // namespace xubinh_server

#endif
