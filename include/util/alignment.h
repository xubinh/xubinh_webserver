#ifndef __XUBINH_SERVER_UTIL_ALIGNMENT
#define __XUBINH_SERVER_UTIL_ALIGNMENT

#include <unistd.h>

namespace xubinh_server {

namespace util {

namespace alignment {

// aligned allocate; promised to return non-null pointer
void *aalloc(size_t alignment, size_t size);

size_t get_level_1_data_cache_line_size();

size_t get_page_size();

// returns the exponent itself (i.e. $e$), not the actual power (i.e. $2^e$);
// the input number must be non-zero
inline size_t get_next_power_of_2(size_t n) {
    // if (n == 0) {
    //     throw std::invalid_argument("power of 2 is undefined for zero");
    // }

    return 32 - __builtin_clz(n - 1);
}

} // namespace alignment

} // namespace util

} // namespace xubinh_server

#endif