#ifndef __XUBINH_SERVER_INET_ADDRESS
#define __XUBINH_SERVER_INET_ADDRESS

#include <arpa/inet.h>
#include <string>

namespace xubinh_server {

class InetAddress {
private:
    using sockaddr_universal_t = sockaddr_storage;

    // [NOTE]: addresses of types `sockaddr_xxx` always store their IPs and
    // ports by network order
    union InetAddressUnion {
        sockaddr_in in_v4;
        sockaddr_in6 in_v6;
        sockaddr_universal_t in_universal;
    };

public:
    enum InetAddressType { IPv4, IPv6 };

    enum InetAddressDirection { LOCAL, PEER };

    // ensuring each address object becomes valid after construction
    InetAddress() = delete;

    // for user
    InetAddress(const std::string &ip, int port, InetAddressType type);

    // for framework
    InetAddress(const sockaddr *address, socklen_t address_length);

    // for framework
    InetAddress(int connect_socketfd, InetAddressDirection direction);

    bool is_ipv4() const {
        return _get_ip_protocol() == AF_INET;
    }

    // for framework
    const sockaddr *get_address() const {
        return reinterpret_cast<const sockaddr *>(&_in_union.in_universal);
    }

    // for framework
    socklen_t get_address_length() const {
        return is_ipv4() ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
    }

    // for user
    std::string get_ip() const;

    // for user
    int get_port() const {
        return static_cast<int>(::ntohs(
            is_ipv4() ? _in_union.in_v4.sin_port : _in_union.in_v6.sin6_port
        ));
    }

    std::string to_string() const {
        return get_ip() + ":" + std::to_string(get_port());
    }

private:
    sa_family_t _get_ip_protocol() const {
        return _in_union.in_universal.ss_family;
    }

    void _check_validity() const;

    InetAddressUnion _in_union;
};

} // namespace xubinh_server

#endif