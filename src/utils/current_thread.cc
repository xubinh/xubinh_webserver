#include <sys/prctl.h>

#include "../../include/utils/current_thread.h"

namespace xubinh_server {

namespace utils {

namespace CurrentThread {

// 用于优化编译器分支预判:
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

thread_local pid_t _tid = 0;
thread_local char _tid_string[32]{};
thread_local size_t _tid_string_length = 0;
thread_local const char *_thread_name = "unknown";

static inline pid_t _get_tid() {
    return static_cast<pid_t>(syscall(SYS_gettid));
}

static void _cache_tid() {
    _tid = _get_tid();
    _tid_string_length =
        snprintf(_tid_string, sizeof _tid_string, "%5d ", _tid);
}

pid_t get_tid() {
    if (UNLIKELY(_tid == 0)) {
        _cache_tid();
    }

    return _tid;
}

static inline void _set_linux_thread_name(const char *thread_name) {
    ::prctl(PR_SET_NAME, thread_name);
}

void set_thread_name(const char *thread_name) {
    _thread_name = thread_name;

    _set_linux_thread_name(_thread_name);
}

} // namespace CurrentThread

} // namespace utils

} // namespace xubinh_server