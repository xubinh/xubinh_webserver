#ifndef XUBINH_SERVER_UTILS_CURRENT_THREAD
#define XUBINH_SERVER_UTILS_CURRENT_THREAD

#include <stdio.h>
#include <string>
#include <sys/syscall.h>
#include <unistd.h>

namespace xubinh_server {

namespace utils {

namespace CurrentThread {

extern thread_local pid_t _tid;

// 用于加快日志消息的构建:
extern thread_local char _tid_string[32];
extern thread_local size_t _tid_string_length;

extern thread_local const char *_thread_name;

pid_t get_tid();

void set_thread_name(const char *thread_name);

inline const char *get_thread_name() {
    return _thread_name;
}

} // namespace CurrentThread

} // namespace utils

} // namespace xubinh_server

#endif