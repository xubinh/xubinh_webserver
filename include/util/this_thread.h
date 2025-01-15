#ifndef __XUBINH_SERVER_UTIL_this_thread
#define __XUBINH_SERVER_UTIL_this_thread

#include <stdio.h>
#include <string>
#include <sys/syscall.h>
#include <unistd.h>

namespace xubinh_server {

namespace util {

namespace this_thread {

pid_t get_tid();

const char *get_tid_string();

int get_tid_string_length();

// [WARN]: the array passed in does not require static storage; it is the
// caller's responsibility to not access it after being destroyed
void set_thread_name(const char *thread_name);

const char *get_thread_name();

} // namespace this_thread

} // namespace util

} // namespace xubinh_server

#endif