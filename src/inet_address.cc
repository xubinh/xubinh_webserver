#include <arpa/inet.h>
#include <cstring>

#include "inet_address.h"
#include "log_builder.h"

namespace xubinh_server {

InetAddress::InetAddress(
    const std::string &ip, int port, InetAddressType type
) {
    if (type == IPv4) {
        // protocol
        _in_union.in_v4.sin_family = AF_INET;

        // IP
        if (!::inet_pton(AF_INET, ip.c_str(), &_in_union.in_v4.sin_addr)) {
            LOG_FATAL << "invalid ip given";
        }

        // port
        _in_union.in_v4.sin_port = ::htons(static_cast<in_port_t>(port));
    }

    else {
        // protocol
        _in_union.in_v6.sin6_family = AF_INET6;

        // IP
        if (!::inet_pton(AF_INET6, ip.c_str(), &_in_union.in_v6.sin6_addr)) {
            LOG_FATAL << "invalid ip given";
        }

        // port
        _in_union.in_v6.sin6_port = ::htons(static_cast<in_port_t>(port));
    }
}

InetAddress::InetAddress(const sockaddr *address, socklen_t address_length) {
    ::memcpy(&_in_union.in_universal, address, address_length);

    _check_validity();
}

InetAddress::InetAddress(int connect_socketfd, InetAddressDirection direction) {
    socklen_t socket_address_len = sizeof(sockaddr_universal_t);

    if (direction == LOCAL) {
        if (::getsockname(
                connect_socketfd,
                reinterpret_cast<sockaddr *>(&_in_union.in_universal),
                &socket_address_len
            )
            == -1) {

            LOG_SYS_FATAL << "failed to get local socket address";
        }
    }

    else {
        if (::getpeername(
                connect_socketfd,
                reinterpret_cast<sockaddr *>(&_in_union.in_universal),
                &socket_address_len
            )
            == -1) {

            LOG_SYS_FATAL << "failed to get peer socket address";
        }
    }

    _check_validity();
}

std::string InetAddress::get_ip() const {
    char ip_c_str[INET6_ADDRSTRLEN + 1]{};

    if (is_ipv4()) {
        ::inet_ntop(
            AF_INET, &_in_union.in_v4.sin_addr, ip_c_str, INET_ADDRSTRLEN
        );
    }

    else {
        ::inet_ntop(
            AF_INET6, &_in_union.in_v6.sin6_addr, ip_c_str, INET6_ADDRSTRLEN
        );
    }

    return std::string(ip_c_str);
}

void InetAddress::_check_validity() const {
    auto protocol = _get_ip_protocol();

    if (protocol != AF_INET && protocol != AF_INET6) {
        LOG_FATAL << "unknown InetAddress protocol";
    }
}

} // namespace xubinh_server