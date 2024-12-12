#include <bits/socket.h>
#include <netinet/in.h>

#include "log_builder.h"
#include "socketfd.h"

namespace xubinh_server {

int Socketfd::get_socketfd_errno(int socketfd) {
    int saved_errno;
    socklen_t saved_errno_len = sizeof(decltype(saved_errno));

    if (::getsockopt(
            socketfd, SOL_SOCKET, SO_ERROR, &saved_errno, &saved_errno_len
        )
        < 0) {
        LOG_SYS_FATAL << "getsockopt failed";
    }
    else {
        return saved_errno;
    }
}

} // namespace xubinh_server