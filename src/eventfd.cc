#include "eventfd.h"
#include "log_builder.h"

namespace xubinh_server {

Eventfd::~Eventfd() {
    _pollable_file_descriptor.detach_from_poller();

    _pollable_file_descriptor.close_fd();

    LOG_INFO << "exit destructor: Eventfd";
}

void Eventfd::start() {
    if (!_message_callback) {
        LOG_FATAL << "missing message callback";
    }

    _pollable_file_descriptor.register_read_event_callback(
        [this](util::TimePoint time_stamp) {
            _read_event_callback(time_stamp);
        }
    );

    _pollable_file_descriptor.enable_read_event();
}

void Eventfd::increment_by_value(uint64_t value) {
    ssize_t bytes_written =
        ::write(_pollable_file_descriptor.get_fd(), &value, sizeof(value));

    if (bytes_written == -1) {
        LOG_SYS_ERROR << "failed writing to eventfd";
    }

    // [TODO]: delete this branch
    else if (bytes_written < static_cast<int>(sizeof(value))) {
        LOG_WARN << "incomplete eventfd write operation";
    }
}

uint64_t Eventfd::_retrieve_the_sum() {
    uint64_t sum;

    auto bytes_read =
        ::read(_pollable_file_descriptor.get_fd(), &sum, sizeof(sum));

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
    else if (bytes_read < static_cast<int>(sizeof(sum))) {
        LOG_WARN << "incomplete eventfd read operation";

        return 0;
    }

    return sum;
}

} // namespace xubinh_server