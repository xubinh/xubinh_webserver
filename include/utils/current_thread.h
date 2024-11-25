#ifndef XUBINH_SERVER_UTILS_CURRENT_THREAD
#define XUBINH_SERVER_UTILS_CURRENT_THREAD

#include <stdio.h>
#include <string>
#include <sys/syscall.h>
#include <unistd.h>

namespace xubinh_server {

namespace utils {

namespace CurrentThread {

pid_t get_tid();

const char *get_tid_string();

int get_tid_string_length();

void set_thread_name(const char *thread_name);

inline const char *get_thread_name();

} // namespace CurrentThread

} // namespace utils

} // namespace xubinh_server

#endif