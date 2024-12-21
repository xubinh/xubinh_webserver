#ifndef __XUBINH_SERVER_UTIL_DATETIME
#define __XUBINH_SERVER_UTIL_DATETIME

#include <cstring>
#include <ctime>
#include <iostream>
#include <string>

namespace xubinh_server {

namespace util {

class Datetime {
public:
    enum class Purpose { RENAMING, PRINTING, NUMBER_OF_ALL_PURPOSES };

    static time_t get_current_time_from_epoch_in_seconds() noexcept {
        return ::time(nullptr);
    }

    static std::string get_datetime_string(
        time_t time_from_epoch_in_seconds, Purpose purpose
    ) noexcept;

    static std::string get_datetime_string(Purpose purpose) noexcept {
        time_t now = get_current_time_from_epoch_in_seconds();

        return get_datetime_string(now, purpose);
    }

    static constexpr size_t DATETIME_STRING_LENGTHES[static_cast<size_t>(
        Purpose::NUMBER_OF_ALL_PURPOSES
    )]{19, 19};
};

} // namespace util

} // namespace xubinh_server

#endif