#include "eventfd.h"
#include "log_builder.h"

namespace xubinh_server {

void Eventfd::increment_by_value(uint64_t value) {
    ssize_t bytes_written = ::write(_fd, &value, sizeof(value));

    if (bytes_written == -1) {
        LOG_SYS_ERROR << "failed writing to eventfd";
    }

    else if (bytes_written < sizeof(value)) {
        LOG_WARN << "incomplete eventfd write operation";
    }
}

uint64_t Eventfd::retrieve_the_sum() {
    uint64_t sum;

    ssize_t bytes_read = ::read(_fd, &sum, sizeof(sum));

    if (bytes_read == -1) {
        LOG_SYS_ERROR << "failed reading from eventfd";

        sum = 0;
    }

    else if (bytes_read < sizeof(sum)) {
        LOG_WARN << "incomplete eventfd read operation";

        sum = 0;
    }

    return sum;
}

} // namespace xubinh_server