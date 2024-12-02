#ifndef XUBINH_SERVER_UTIL_TIME_POINT
#define XUBINH_SERVER_UTIL_TIME_POINT

#include <cstdint>

namespace xubinh_server {

namespace util {

struct TimeInterval {
    TimeInterval() = default;

    TimeInterval(const TimeInterval &other) : nanoseconds(other.nanoseconds) {
    }

    TimeInterval(int64_t nanoseconds_input) : nanoseconds(nanoseconds_input) {
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

    TimeInterval operator+(const TimeInterval &other) {
        return TimeInterval(*this) += other;
    }

    TimeInterval operator-(const TimeInterval &other) {
        return TimeInterval(*this) -= other;
    }

    bool operator<(const TimeInterval &other) {
        return nanoseconds < other.nanoseconds;
    }

    bool operator<=(const TimeInterval &other) {
        return nanoseconds <= other.nanoseconds;
    }

    bool operator>(const TimeInterval &other) {
        return nanoseconds > other.nanoseconds;
    }

    bool operator>=(const TimeInterval &other) {
        return nanoseconds >= other.nanoseconds;
    }

    bool operator==(const TimeInterval &other) {
        return nanoseconds == other.nanoseconds;
    }

    int64_t nanoseconds = 0;
};

struct TimePoint {
    static int64_t get_nanoseconds_from_epoch();

    TimePoint() : nanoseconds_from_epoch(get_nanoseconds_from_epoch()) {
    }

    TimePoint(const TimePoint &other)
        : nanoseconds_from_epoch(other.nanoseconds_from_epoch) {
    }

    TimePoint &operator+=(const TimeInterval &time_interval) {
        nanoseconds_from_epoch += time_interval.nanoseconds;

        return *this;
    }

    TimePoint &operator-=(const TimeInterval &time_interval) {
        nanoseconds_from_epoch -= time_interval.nanoseconds;

        return *this;
    }

    TimePoint operator+(const TimeInterval &time_interval) {
        return TimePoint(*this) += time_interval;
    }

    TimePoint operator-(const TimeInterval &time_interval) {
        return TimePoint(*this) -= time_interval;
    }

    TimeInterval operator-(const TimePoint &time_point) {
        return nanoseconds_from_epoch - time_point.nanoseconds_from_epoch;
    }

    bool operator<(const TimePoint &other) {
        return nanoseconds_from_epoch < other.nanoseconds_from_epoch;
    }

    bool operator<=(const TimePoint &other) {
        return nanoseconds_from_epoch <= other.nanoseconds_from_epoch;
    }

    bool operator>(const TimePoint &other) {
        return nanoseconds_from_epoch > other.nanoseconds_from_epoch;
    }

    bool operator>=(const TimePoint &other) {
        return nanoseconds_from_epoch >= other.nanoseconds_from_epoch;
    }

    bool operator==(const TimePoint &other) {
        return nanoseconds_from_epoch == other.nanoseconds_from_epoch;
    }

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