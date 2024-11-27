#ifndef XUBINH_SERVER_UTIL_ERRNO
#define XUBINH_SERVER_UTIL_ERRNO

#include <string>

namespace xubinh_server {

namespace util {

std::string strerror_tl(int saved_errno);

} // namespace util

} // namespace xubinh_server

#endif