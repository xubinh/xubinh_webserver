#ifndef __XUBINH_SERVER_UTIL_FILE
#define __XUBINH_SERVER_UTIL_FILE

#include <string>

namespace xubinh_server {

namespace util {

// not thread-safe
class AppendOnlyPhysicalFile {
public:
    explicit AppendOnlyPhysicalFile(const std::string &base_name);

    // no copy
    AppendOnlyPhysicalFile(const AppendOnlyPhysicalFile &) = delete;
    AppendOnlyPhysicalFile &operator=(const AppendOnlyPhysicalFile &) = delete;

    // no move
    AppendOnlyPhysicalFile(AppendOnlyPhysicalFile &&) = delete;
    AppendOnlyPhysicalFile &operator=(AppendOnlyPhysicalFile &&) = delete;

    ~AppendOnlyPhysicalFile();

    void append(const char *data, size_t data_size);

    // need a flush method since a user-space buffer `FILE *` is used internally
    void flush();

    size_t get_total_number_of_bytes_written() {
        return _total_number_of_bytes_written;
    }

private:
    FILE *_file_ptr;
    char _buffer[64 * 1000]; // for replacing the internal default buffer that
                             // `_file_ptr` uses
    size_t _total_number_of_bytes_written;
};

} // namespace util

} // namespace xubinh_server

#endif