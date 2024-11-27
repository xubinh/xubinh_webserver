#ifndef XUBINH_SERVER_UTIL_CURRENT_THREAD
#define XUBINH_SERVER_UTIL_CURRENT_THREAD

#include <stdio.h>
#include <string>
#include <sys/syscall.h>
#include <unistd.h>

namespace xubinh_server {

namespace util {

namespace current_thread {

void reset_tid();

pid_t get_tid();

const char *get_tid_string();

int get_tid_string_length();

void set_thread_name(const char *thread_name);

inline const char *get_thread_name();

} // namespace current_thread

} // namespace util

} // namespace xubinh_server

#endif