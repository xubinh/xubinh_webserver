#include "util/mutex.h"
#include "util/errno.h"

namespace xubinh_server {

namespace util {

Mutex::Mutex() noexcept {
    int saved_errno = ::pthread_mutex_init(&_pthread_mutex, nullptr);

    if (saved_errno != 0) {
        ::fprintf(
            stderr,
            "@xubinh_server::util::Mutex::Mutex: pthread_mutex_init "
            "failed: %s (errno=%d)\n",
            strerror_tl(errno).c_str(),
            saved_errno
        );

        ::abort();
    }
}

Mutex::~Mutex() noexcept {
    int saved_errno = ::pthread_mutex_destroy(&_pthread_mutex);

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

void Mutex::lock() noexcept {
    int saved_errno = ::pthread_mutex_lock(&_pthread_mutex);

    if (saved_errno != 0) {
        ::fprintf(
            stderr,
            "@xubinh_server::util::Mutex::lock: pthread_mutex_lock failed: "
            "%s (errno=%d)\n",
            strerror_tl(errno).c_str(),
            saved_errno
        );

        ::abort();
    }
}

bool Mutex::try_lock() noexcept {
    int saved_errno = ::pthread_mutex_trylock(&_pthread_mutex);

    if (saved_errno == 0) {
        return true;
    }

    else if (saved_errno == EBUSY) {
        return false;
    }

    else {
        ::fprintf(
            stderr,
            "@xubinh_server::util::Mutex::try_lock: pthread_mutex_trylock "
            "failed: %s (errno=%d)\n",
            strerror_tl(errno).c_str(),
            saved_errno
        );

        ::abort();
    }
}

void Mutex::unlock() noexcept {
    int saved_errno = pthread_mutex_unlock(&_pthread_mutex);

    if (saved_errno != 0) {
        ::fprintf(
            stderr,
            "@xubinh_server::util::Mutex::unlock: pthread_mutex_unlock "
            "failed: %s (errno=%d)\n",
            strerror_tl(errno).c_str(),
            saved_errno
        );

        ::abort();
    }
}

} // namespace util

} // namespace xubinh_server