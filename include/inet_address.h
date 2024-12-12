#ifndef XUBINH_SERVER_INET_ADDRESS
#define XUBINH_SERVER_INET_ADDRESS

#include <bits/socket.h>
#include <netinet/in.h>
#include <string>

#include "log_builder.h"

namespace xubinh_server {

class InetAddress {
public:
    using sockaddr_unknown_t = sockaddr_in6;

    enum InetAddressType { IPv4, IPv6 };

    InetAddress() : _in_unknown{} {
    }

    // for user
    InetAddress(const std::string &ip, int port, InetAddressType inet_type);

    // for framework
    InetAddress(const sockaddr *address, socklen_t address_length) {
        ::memcpy(&_in_unknown, address, address_length);
    }

    bool is_ipv4() const;

    // for framework
    const sockaddr *get_address() const {
        // dummy code, only for checking initialization
        is_ipv4();

        return reinterpret_cast<const sockaddr *>(&_in_unknown);
    }

    // for framework
    socklen_t get_address_length() const {
        return is_ipv4() ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
    }

    // for user
    std::string get_ip() const;

    // for user
    int get_port() const {
        return static_cast<int>(
            ::ntohs(is_ipv4() ? _in.sin_port : _in6.sin6_port)
        );
    }

private:
    static sa_family_t _get_sa_family(const void *address) {
        return *reinterpret_cast<const sa_family_t *>(address);
    }

    // always store as network order
    union {
        sockaddr_in _in;
        sockaddr_in6 _in6;
        sockaddr_unknown_t _in_unknown;
    };
};

} // namespace xubinh_server

#endif