#include <cstring> // strerror_r

#include "util/errno.h"

namespace xubinh_server {

namespace util {

thread_local char strerror_buffer_tl[512];

std::string strerror_tl(int saved_errno) {
    // 使用的是 GNU 版本, 在提供的缓冲区空间不够的时候会退化为线程不安全的版本:
    return ::strerror_r(
        saved_errno, strerror_buffer_tl, sizeof strerror_buffer_tl
    );
}

} // namespace util

} // namespace xubinh_server