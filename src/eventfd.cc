#include "eventfd.h"
#include "log_builder.h"

namespace xubinh_server {

// thread-safe
void Eventfd::increment_by_value(uint64_t value) {
    ssize_t bytes_written = ::write(_fd, &value, sizeof(value));

    if (bytes_written == -1) {
        LOG_SYS_ERROR << "failed writing to eventfd";
    }

    // [TODO]: delete this branch
    else if (bytes_written < sizeof(value)) {
        LOG_WARN << "incomplete eventfd write operation";
    }
}

// thread-safe
uint64_t Eventfd::retrieve_the_sum() {
    uint64_t sum;

    auto bytes_read = ::read(_fd, &sum, sizeof(sum));

    if (bytes_read == -1) {
        if (errno == EAGAIN) {
            LOG_WARN << "eventfd is read before being notified";
        }

        else {

            LOG_SYS_ERROR << "failed reading from eventfd";
        }

        return 0;
    }

    // [TODO]: delete this branch
    else if (bytes_read < sizeof(sum)) {
        LOG_WARN << "incomplete eventfd read operation";

        return 0;
    }

    return sum;
}

} // namespace xubinh_server