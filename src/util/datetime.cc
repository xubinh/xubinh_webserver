#include "util/datetime.h"

namespace xubinh_server {

namespace util {

std::string
Datetime::get_datetime_string(const DatetimePurpose &datetime_purpose) {
    time_t now = get_current_time_from_epoch_in_seconds();
    tm tm{};

    localtime_r(&now, &tm);

    char buffer[32];

    // for renaming physical files (e.g. `2024_01_01_13_00_23`):
    if (datetime_purpose == DatetimePurpose::RENAMING) {
        strftime(buffer, sizeof(buffer), "%Y_%m_%d_%H_%M_%S", &tm);
    }

    // for printing out into terminal or as log (e.g. `2024-01-01 13:00:23`):
    else if (datetime_purpose == DatetimePurpose::PRINTING) {
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
    }

    return buffer;
}

} // namespace util

} // namespace xubinh_server