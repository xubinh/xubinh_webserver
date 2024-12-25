#ifndef __XUBINH_SERVER_UTIL_TIME_POINT
#define __XUBINH_SERVER_UTIL_TIME_POINT

#include <cstdint>
#include <string>
#include <sys/timerfd.h>

namespace xubinh_server {

namespace util {

struct TimeInterval {
    TimeInterval() noexcept = default;

    TimeInterval(int64_t nanoseconds_input) noexcept
        : nanoseconds(nanoseconds_input) {
    }

    operator bool() const noexcept {
        return static_cast<bool>(nanoseconds);
    }

    TimeInterval &operator+=(const TimeInterval &other) noexcept {
        nanoseconds += other.nanoseconds;

        return *this;
    }

    TimeInterval &operator-=(const TimeInterval &other) noexcept {
        nanoseconds -= other.nanoseconds;

        return *this;
    }

    TimeInterval &operator*=(int64_t multiplier) noexcept {
        nanoseconds *= multiplier;

        return *this;
    }

    TimeInterval operator+(const TimeInterval &other) const noexcept {
        return TimeInterval(*this) += other;
    }

    TimeInterval operator-(const TimeInterval &other) const noexcept {
        return TimeInterval(*this) -= other;
    }

    TimeInterval operator*(int64_t multiplier) const noexcept {
        return TimeInterval(*this) *= multiplier;
    }

    bool operator<(const TimeInterval &other) const noexcept {
        return nanoseconds < other.nanoseconds;
    }

    bool operator<=(const TimeInterval &other) const noexcept {
        return nanoseconds <= other.nanoseconds;
    }

    bool operator>(const TimeInterval &other) const noexcept {
        return nanoseconds > other.nanoseconds;
    }

    bool operator>=(const TimeInterval &other) const noexcept {
        return nanoseconds >= other.nanoseconds;
    }

    bool operator==(const TimeInterval &other) const noexcept {
        return nanoseconds == other.nanoseconds;
    }

    bool operator!=(const TimeInterval &other) const noexcept {
        return nanoseconds != other.nanoseconds;
    }

    static constexpr int64_t FOREVER = 0x7fffffffffffffff;

    int64_t nanoseconds = 0;
};

struct TimePoint {
    enum class Purpose { RENAMING, PRINTING, NUMBER_OF_ALL_PURPOSES };

    static int64_t get_nanoseconds_from_epoch() noexcept;

    static void get_timespec(
        int64_t nanoseconds_from_epoch, timespec *time_specification
    ) noexcept;

    static std::string get_datetime_string(
        int64_t nanoseconds_from_epoch, Purpose purpose
    ) noexcept;

    static std::string get_datetime_string(Purpose purpose) noexcept {
        int64_t now = get_nanoseconds_from_epoch();

        return get_datetime_string(now, purpose);
    }

    TimePoint() noexcept
        : nanoseconds_from_epoch(get_nanoseconds_from_epoch()) {
    }

    TimePoint(int64_t nanoseconds_from_epoch_input) noexcept
        : nanoseconds_from_epoch(nanoseconds_from_epoch_input) {
    }

    void to_timespec(timespec *time_specification) const noexcept {
        get_timespec(nanoseconds_from_epoch, time_specification);
    }

    std::string to_datetime_string(Purpose purpose) const noexcept {
        return get_datetime_string(nanoseconds_from_epoch, purpose);
    }

    TimePoint &operator+=(TimeInterval time_interval) noexcept {
        nanoseconds_from_epoch += time_interval.nanoseconds;

        return *this;
    }

    TimePoint &operator-=(TimeInterval time_interval) noexcept {
        nanoseconds_from_epoch -= time_interval.nanoseconds;

        return *this;
    }

    TimePoint operator+(TimeInterval time_interval) const noexcept {
        return TimePoint(*this) += time_interval;
    }

    TimePoint operator-(TimeInterval time_interval) const noexcept {
        return TimePoint(*this) -= time_interval;
    }

    TimeInterval operator-(const TimePoint &time_point) const noexcept {
        return nanoseconds_from_epoch - time_point.nanoseconds_from_epoch;
    }

    bool operator<(const TimePoint &other) const noexcept {
        return nanoseconds_from_epoch < other.nanoseconds_from_epoch;
    }

    bool operator<=(const TimePoint &other) const noexcept {
        return nanoseconds_from_epoch <= other.nanoseconds_from_epoch;
    }

    bool operator>(const TimePoint &other) const noexcept {
        return nanoseconds_from_epoch > other.nanoseconds_from_epoch;
    }

    bool operator>=(const TimePoint &other) const noexcept {
        return nanoseconds_from_epoch >= other.nanoseconds_from_epoch;
    }

    bool operator==(const TimePoint &other) const noexcept {
        return nanoseconds_from_epoch == other.nanoseconds_from_epoch;
    }

    bool operator!=(const TimePoint &other) const noexcept {
        return nanoseconds_from_epoch != other.nanoseconds_from_epoch;
    }

    bool operator==(const int64_t &other_nanoseconds_from_epoch
    ) const noexcept {
        return nanoseconds_from_epoch == other_nanoseconds_from_epoch;
    }

    bool operator!=(const int64_t &other_nanoseconds_from_epoch
    ) const noexcept {
        return nanoseconds_from_epoch != other_nanoseconds_from_epoch;
    }

    static constexpr size_t TIME_POINT_STRING_LENGTHES[static_cast<size_t>(
        Purpose::NUMBER_OF_ALL_PURPOSES
    )]{29, 29};

    static constexpr int64_t FOREVER = 0x7fffffffffffffff;

    int64_t nanoseconds_from_epoch;
};

} // namespace util

} // namespace xubinh_server

namespace std {

template <>
struct hash<xubinh_server::util::TimePoint> {
    std::size_t operator()(xubinh_server::util::TimePoint time_point) const {
        return std::hash<int64_t>()(time_point.nanoseconds_from_epoch);
    }
};

} // namespace std

#endif