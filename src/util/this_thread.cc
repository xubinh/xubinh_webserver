#include <sys/prctl.h>

#include "util/this_thread.h"

namespace xubinh_server {

namespace util {

namespace this_thread {

namespace {

thread_local pid_t _tid = 0; // the TID of the first process (i.e. init) is 1

// for speeding up construction of single log strings
thread_local char _tid_string[32]{};
thread_local size_t _tid_string_length = 0;

thread_local const char *_thread_name = "unknown";

pid_t _get_tid() {
    // for compatibility with glibc before v2.30; see man page of `gettid()`
    return static_cast<pid_t>(syscall(SYS_gettid));
}

void _cache_tid() {
    // number representation
    _tid = _get_tid();

    // string representation
    _tid_string_length =
        ::snprintf(_tid_string, sizeof _tid_string, "%5d", _tid);
}

void _set_linux_thread_name(const char *thread_name) {
    ::prctl(PR_SET_NAME, thread_name);
}

} // namespace

pid_t get_tid() {
    if (__builtin_expect(_tid == 0, false)) {
        _cache_tid();
    }

    return _tid;
}

const char *get_tid_string() {
    if (__builtin_expect(_tid == 0, false)) {
        _cache_tid();
    }

    return _tid_string;
}

int get_tid_string_length() {
    if (__builtin_expect(_tid == 0, false)) {
        _cache_tid();
    }

    return _tid_string_length;
}

void set_thread_name(const char *thread_name) {
    _thread_name = thread_name;

    _set_linux_thread_name(thread_name);
}

const char *get_thread_name() {
    return _thread_name;
}

} // namespace this_thread

} // namespace util

} // namespace xubinh_server