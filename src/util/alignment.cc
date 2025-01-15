#include <cerrno>
#include <cstdio>
#include <cstdlib>

#include "util/alignment.h"
#include "util/errno.h"

namespace xubinh_server {

namespace util {

namespace alignment {

void *aalloc(size_t alignment, size_t size) {
#ifndef _ISOC11_SOURCE
    ::fprintf(
        stderr, "@xubinh_server::util::alignment::aalloc: not supported\n"
    );

    ::abort();
#else
    void *ptr = ::aligned_alloc(alignment, size);

    if (ptr == NULL) {
        ::fprintf(
            stderr,
            "@xubinh_server::util::alignment::aalloc: %s (errno=%d)\n",
            strerror_tl(errno).c_str(),
            errno
        );

        ::abort();
    }

    return ptr;
#endif
}

size_t get_level_1_data_cache_line_size() {
#ifndef _SC_LEVEL1_DCACHE_LINESIZE
    ::fprintf(
        stderr,
        "@xubinh_server::util::alignment::get_level_1_data_cache_line_size: "
        "not supported\n"
    );

    ::abort();
#else
    errno = 0;

    long level_1_data_cache_line_size = ::sysconf(_SC_LEVEL1_DCACHE_LINESIZE);

    if (level_1_data_cache_line_size == -1) {
        if (errno == 0) {
            ::fprintf(
                stderr,
                "@xubinh_server::util::alignment::get_level_1_data_cache_line_"
                "size: the limit is indeterminate\n"
            );

            ::abort();
        }

        else {
            ::fprintf(
                stderr,
                "@xubinh_server::util::alignment::get_level_1_data_cache_line_"
                "size: %s (errno=%d)\n",
                strerror_tl(errno).c_str(),
                errno
            );

            ::abort();
        }
    }

    return level_1_data_cache_line_size;
#endif
}

size_t get_page_size() {
#ifndef _SC_PAGESIZE
    ::fprintf(
        stderr,
        "@xubinh_server::util::alignment::get_page_size: "
        "not supported\n"
    );

    ::abort();
#else
    errno = 0;

    long page_size = ::sysconf(_SC_PAGESIZE);

    if (page_size == -1) {
        if (errno == 0) {
            ::fprintf(
                stderr,
                "@xubinh_server::util::alignment::get_page_size"
                "size: the limit is indeterminate\n"
            );

            ::abort();
        }

        else {
            ::fprintf(
                stderr,
                "@xubinh_server::util::alignment::get_page_size"
                "size: %s (errno=%d)\n",
                strerror_tl(errno).c_str(),
                errno
            );

            ::abort();
        }
    }

    return page_size;
#endif
}

} // namespace alignment

} // namespace util

} // namespace xubinh_server