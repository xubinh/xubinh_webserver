#include <arpa/inet.h>
#include <cstring>

#include "inet_address.h"
#include "log_builder.h"

namespace xubinh_server {

InetAddress::InetAddress(
    const std::string &ip, int port, InetAddressType inet_type
) {
    if (inet_type == IPv4) {
        // protocol
        _in.sin_family = AF_INET;

        // IP
        if (!::inet_pton(AF_INET, ip.c_str(), &_in.sin_addr)) {
            LOG_FATAL << "invalid ip given";
        }

        // port
        _in.sin_port = static_cast<in_port_t>(port);
    }

    else {
        // protocol
        _in6.sin6_family = AF_INET6;

        // IP
        if (!::inet_pton(AF_INET6, ip.c_str(), &_in6.sin6_addr)) {
            LOG_FATAL << "invalid ip given";
        }

        // port
        _in6.sin6_port = static_cast<in_port_t>(port);
    }
}

std::string InetAddress::get_ip() const {
    char ip_c_str[INET6_ADDRSTRLEN + 1]{};

    if (is_ipv4()) {
        ::inet_ntop(AF_INET, &_in.sin_addr, ip_c_str, INET_ADDRSTRLEN);
    }
    else {
        ::inet_ntop(AF_INET6, &_in6.sin6_addr, ip_c_str, INET6_ADDRSTRLEN);
    }

    return std::string(ip_c_str);
}

} // namespace xubinh_server