#ifndef __XUBINH_SERVER_UTIL_ALIGNMENT
#define __XUBINH_SERVER_UTIL_ALIGNMENT

#include <unistd.h>

namespace xubinh_server {

namespace util {

namespace alignment {

// aligned allocate
void *aalloc(size_t alignment, size_t size);

size_t get_level_1_data_cache_line_size();

size_t get_page_size();

} // namespace alignment

} // namespace util

} // namespace xubinh_server

#endif