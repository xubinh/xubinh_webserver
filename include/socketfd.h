#ifndef XUBINH_SERVER_SOCKETFD
#define XUBINH_SERVER_SOCKETFD

namespace xubinh_server {

// implemented as a drawer class for containing universal socketfd APIs
struct Socketfd {
    static int create_socketfd();

    static int get_socketfd_errno(int socketfd);
};

} // namespace xubinh_server

#endif