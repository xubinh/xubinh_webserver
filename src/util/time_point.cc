#include <cstdint>
#include <ctime>
#include <unordered_map>

#include "log_builder.h"
#include "util/time_point.h"

namespace xubinh_server {

namespace util {

int64_t TimePoint::get_nanoseconds_from_epoch() noexcept {
    timespec ts;

    if (::clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        LOG_SYS_FATAL << "unknown error when calling clock_gettime";
    }

    return static_cast<int64_t>(ts.tv_sec) * 1000 * 1000 * 1000
           + static_cast<int64_t>(ts.tv_nsec);
}

std::string TimePoint::get_datetime_string(
    int64_t nanoseconds_from_epoch, Purpose purpose
) noexcept {
    timespec time_specification;

    get_timespec(nanoseconds_from_epoch, &time_specification);

    tm tm_info;

    ::localtime_r(&time_specification.tv_sec, &tm_info);

    char buffer[32];

    size_t actual_bytes_written;

    // for renaming physical files (e.g. `2024_01_01_13_00_23`):
    if (purpose == Purpose::RENAMING) {
        actual_bytes_written =
            ::strftime(buffer, sizeof(buffer), "%Y_%m_%d_%H_%M_%S", &tm_info);
    }

    // for printing out into terminal or as log (e.g. `2024-01-01 13:00:23`):
    else {
        actual_bytes_written =
            ::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm_info);
    }

    actual_bytes_written += ::snprintf(
        buffer + actual_bytes_written,
        sizeof(buffer),
        ".%09ld",
        time_specification.tv_nsec
    );

    return {buffer, actual_bytes_written};
}

} // namespace util

} // namespace xubinh_server
