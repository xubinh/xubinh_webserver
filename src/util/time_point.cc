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

    return static_cast<int64_t>(ts.tv_sec) * TimeInterval::SECOND
           + static_cast<int64_t>(ts.tv_nsec);
}

// [TODO]: use a fixed std::string to avoid the additional copying when
// converting from char[] to std::string
std::string TimePoint::get_datetime_string(
    int64_t nanoseconds_from_epoch, Purpose purpose
) noexcept {
    timespec time_specification;

    get_timespec(nanoseconds_from_epoch, &time_specification);

    // if the newest epoch seconds in the cache is older than the given target
    // then the cache is outdated and needs to be updated
    if (_seconds_from_epoch_cache != time_specification.tv_sec) {
        _seconds_from_epoch_cache = time_specification.tv_sec;

        tm tm_info;

        ::localtime_r(&time_specification.tv_sec, &tm_info);

        // string for renaming physical files (e.g. `2024_01_01_13_00_23`)
        _length_of_datetime_string_for_renaming_cache = ::strftime(
            _datetime_string_for_renaming_cache,
            sizeof(_datetime_string_for_renaming_cache),
            "%Y_%m_%d_%H_%M_%S",
            &tm_info
        );

        // string for printing out to the terminal or as log (e.g. `2024-01-01
        // 13:00:23`)
        _length_of_datetime_string_for_printing_cache = ::strftime(
            _datetime_string_for_printing_cache,
            sizeof(_datetime_string_for_printing_cache),
            "%Y-%m-%d %H:%M:%S",
            &tm_info
        );
    }

    char *actual_datetime_string;
    size_t actual_datetime_string_length;

    if (purpose == Purpose::RENAMING) {
        actual_datetime_string = _datetime_string_for_renaming_cache;
        actual_datetime_string_length =
            _length_of_datetime_string_for_renaming_cache;

        actual_datetime_string_length += ::snprintf(
            _datetime_string_for_renaming_cache
                + _length_of_datetime_string_for_renaming_cache,
            sizeof(_datetime_string_for_renaming_cache),
            ".%09ld",
            time_specification.tv_nsec
        );
    }

    else /* if (purpose == Purpose::PRINTING) */ {
        actual_datetime_string = _datetime_string_for_printing_cache;
        actual_datetime_string_length =
            _length_of_datetime_string_for_printing_cache;

        actual_datetime_string_length += ::snprintf(
            _datetime_string_for_printing_cache
                + _length_of_datetime_string_for_printing_cache,
            sizeof(_datetime_string_for_printing_cache),
            ".%09ld",
            time_specification.tv_nsec
        );
    }

    return {actual_datetime_string, actual_datetime_string_length};
}

thread_local decltype(timespec().tv_sec
) TimePoint::_seconds_from_epoch_cache{0};
thread_local char TimePoint::_datetime_string_for_renaming_cache[32]{};
thread_local size_t TimePoint::_length_of_datetime_string_for_renaming_cache{0};
thread_local char TimePoint::_datetime_string_for_printing_cache[32]{};
thread_local size_t TimePoint::_length_of_datetime_string_for_printing_cache{0};

} // namespace util

} // namespace xubinh_server
