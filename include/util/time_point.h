#ifndef __XUBINH_SERVER_UTIL_TIME_POINT
#define __XUBINH_SERVER_UTIL_TIME_POINT

#include <cstdint>
#include <sys/timerfd.h>

#include "util/datetime.h"

namespace xubinh_server {

namespace util {

struct TimeInterval {
    constexpr TimeInterval() = default;

    constexpr TimeInterval(int64_t nanoseconds_input)
        : nanoseconds(nanoseconds_input) {
    }

    operator bool() const {
        return static_cast<bool>(nanoseconds);
    }

    TimeInterval &operator+=(const TimeInterval &other) {
        nanoseconds += other.nanoseconds;

        return *this;
    }

    TimeInterval &operator-=(const TimeInterval &other) {
        nanoseconds -= other.nanoseconds;

        return *this;
    }

    TimeInterval &operator*=(int64_t multiplier) {
        nanoseconds *= multiplier;

        return *this;
    }

    TimeInterval operator+(const TimeInterval &other) const {
        return TimeInterval(*this) += other;
    }

    TimeInterval operator-(const TimeInterval &other) const {
        return TimeInterval(*this) -= other;
    }

    TimeInterval operator*(int64_t multiplier) const {
        return TimeInterval(*this) *= multiplier;
    }

    bool operator<(const TimeInterval &other) const {
        return nanoseconds < other.nanoseconds;
    }

    bool operator<=(const TimeInterval &other) const {
        return nanoseconds <= other.nanoseconds;
    }

    bool operator>(const TimeInterval &other) const {
        return nanoseconds > other.nanoseconds;
    }

    bool operator>=(const TimeInterval &other) const {
        return nanoseconds >= other.nanoseconds;
    }

    bool operator==(const TimeInterval &other) const {
        return nanoseconds == other.nanoseconds;
    }

    int64_t nanoseconds = 0;
};

struct TimePoint {
    using DatetimePurpose = util::DatetimePurpose;

    static int64_t get_nanoseconds_from_epoch();

    TimePoint() : nanoseconds_from_epoch(get_nanoseconds_from_epoch()) {
    }

    TimePoint(int64_t nanoseconds_from_epoch_input)
        : nanoseconds_from_epoch(nanoseconds_from_epoch_input) {
    }

    void to_timespec(timespec *time_specification) const;

    std::string to_datetime_string(const DatetimePurpose &datetime_purpose
    ) const;

    TimePoint &operator+=(const TimeInterval &time_interval) {
        nanoseconds_from_epoch += time_interval.nanoseconds;

        return *this;
    }

    TimePoint &operator-=(const TimeInterval &time_interval) {
        nanoseconds_from_epoch -= time_interval.nanoseconds;

        return *this;
    }

    TimePoint operator+(const TimeInterval &time_interval) const {
        return TimePoint(*this) += time_interval;
    }

    TimePoint operator-(const TimeInterval &time_interval) const {
        return TimePoint(*this) -= time_interval;
    }

    TimeInterval operator-(const TimePoint &time_point) const {
        return nanoseconds_from_epoch - time_point.nanoseconds_from_epoch;
    }

    bool operator<(const TimePoint &other) const {
        return nanoseconds_from_epoch < other.nanoseconds_from_epoch;
    }

    bool operator<=(const TimePoint &other) const {
        return nanoseconds_from_epoch <= other.nanoseconds_from_epoch;
    }

    bool operator>(const TimePoint &other) const {
        return nanoseconds_from_epoch > other.nanoseconds_from_epoch;
    }

    bool operator>=(const TimePoint &other) const {
        return nanoseconds_from_epoch >= other.nanoseconds_from_epoch;
    }

    bool operator==(const TimePoint &other) const {
        return nanoseconds_from_epoch == other.nanoseconds_from_epoch;
    }

    bool operator!=(const TimePoint &other) const {
        return !operator==(other);
    }

    bool operator==(const int64_t &other_nanoseconds_from_epoch) const {
        return nanoseconds_from_epoch == other_nanoseconds_from_epoch;
    }

    bool operator!=(const int64_t &other_nanoseconds_from_epoch) const {
        return !operator==(other_nanoseconds_from_epoch);
    }

    static constexpr int64_t FOREVER = 0x7fffffffffffffff;

    int64_t nanoseconds_from_epoch = 0;
};

} // namespace util

} // namespace xubinh_server

namespace std {

template <>
struct hash<xubinh_server::util::TimePoint> {
    std::size_t operator()(const xubinh_server::util::TimePoint &time_point
    ) const {
        return std::hash<int64_t>()(time_point.nanoseconds_from_epoch);
    }
};

} // namespace std

#endif