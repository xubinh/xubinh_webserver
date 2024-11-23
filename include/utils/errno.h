#ifndef XUBINH_SERVER_UTILS_ERRNO
#define XUBINH_SERVER_UTILS_ERRNO

#include <string>

namespace xubinh_server {

namespace utils {

std::string strerror_tl(int saved_errno);

} // namespace utils

} // namespace xubinh_server

#endif