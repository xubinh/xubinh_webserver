#ifndef XUBINH_SERVER_INET_ADDRESS
#define XUBINH_SERVER_INET_ADDRESS

#include <bits/socket.h>
#include <netinet/in.h>
#include <string>

#include "log_builder.h"

namespace xubinh_server {

class InetAddress {
public:
    using sockaddr_unknown_t = sockaddr_storage;

    enum InetAddressType { IPv4, IPv6 };

    enum InetAddressDirection { LOCAL, PEER };

    // for user
    InetAddress(const std::string &ip, int port, InetAddressType type);

    // for framework
    InetAddress(const sockaddr *address, socklen_t address_length) {
        ::memcpy(&_in_unknown, address, address_length);

        _check_validity();
    }

    InetAddress(int connect_socketfd, InetAddressDirection direction);

    bool is_ipv4() const {
        return _get_sa_family(&_in_unknown) == AF_INET;
    }

    // for framework
    const sockaddr *get_address() const {
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

    void _check_validity() const {
        auto protocol = _get_sa_family(&_in_unknown);

        if (protocol != AF_INET && protocol != AF_INET6) {
            LOG_FATAL << "unknown InetAddress protocol";
        }
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