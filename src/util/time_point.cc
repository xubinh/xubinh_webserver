#include <cstdint>
#include <ctime>
#include <unordered_map>

#include "util/time_point.h"

namespace xubinh_server {

namespace util {

int64_t TimePoint::get_nanoseconds_from_epoch() {
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);

    return static_cast<int64_t>(ts.tv_sec) * 1000000000
           + static_cast<int64_t>(ts.tv_nsec);
}
} // namespace util

} // namespace xubinh_server
