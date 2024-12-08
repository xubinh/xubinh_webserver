#include <fcntl.h>

#include "log_builder.h"
#include "pollable_file_descriptor.h"

namespace xubinh_server {

void PollableFileDescriptor::set_fd_as_nonblocking(int fd) {
    auto flags = fcntl(fd, F_GETFL, 0);

    if (flags == -1) {
        LOG_SYS_FATAL << "fcntl F_GETFL";
    }

    flags |= O_NONBLOCK;

    if (fcntl(fd, F_SETFL, flags) == -1) {
        LOG_SYS_FATAL << "fcntl F_SETFL";
    }

    return;
}

} // namespace xubinh_server