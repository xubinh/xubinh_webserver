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

void TimePoint::to_timespec(timespec *time_specification) const {
    time_specification->tv_sec =
        static_cast<decltype(time_specification->tv_sec)>(
            nanoseconds_from_epoch / static_cast<int64_t>(1000000000)
        );

    time_specification->tv_nsec =
        static_cast<decltype(time_specification->tv_nsec)>(
            nanoseconds_from_epoch % static_cast<int64_t>(1000000000)
        );
}

std::string
TimePoint::to_datetime_string(const DatetimePurpose &datetime_purpose) const {
    auto time_from_epoch_in_seconds = static_cast<time_t>(
        nanoseconds_from_epoch / static_cast<int64_t>(1000 * 1000 * 1000)
    );

    return util::Datetime::get_datetime_string(
        time_from_epoch_in_seconds, datetime_purpose
    );
}

} // namespace util

} // namespace xubinh_server
