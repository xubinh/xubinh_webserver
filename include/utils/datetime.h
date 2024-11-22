#ifndef XUBINH_SERVER_UTILS_DATETIME
#define XUBINH_SERVER_UTILS_DATETIME

#include <cstring> // std::memset
#include <ctime>
#include <iostream>
#include <string>

namespace xubinh_server {

namespace utils {

enum class DatetimePurpose { RENAMING, PRINTING };

class Datetime {
public:
    static std::time_t get_current_time_from_epoch_in_seconds() {
        return std::time(nullptr);
    }

    static std::string
    get_datetime_string(const DatetimePurpose &datetime_purpose);
};

} // namespace utils

} // namespace xubinh_server

#endif