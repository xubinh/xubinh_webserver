#include "util/datetime.h"

namespace xubinh_server {

namespace util {

std::string Datetime::get_datetime_string(
    time_t time_from_epoch_in_seconds, Purpose purpose
) noexcept {
    tm tm_info;

    ::localtime_r(&time_from_epoch_in_seconds, &tm_info);

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

    return {buffer, actual_bytes_written};
}

} // namespace util

} // namespace xubinh_server