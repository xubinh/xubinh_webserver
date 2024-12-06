#include <cstring>

#include "util/errno.h"

namespace xubinh_server {

namespace util {

thread_local char strerror_buffer_tl[512];

std::string strerror_tl(int saved_errno) {
    // GNU version, degenerates to the default, not thread-safe version if the
    // given buffer is too small
    return strerror_r(
        saved_errno, strerror_buffer_tl, sizeof strerror_buffer_tl
    );
}

} // namespace util

} // namespace xubinh_server