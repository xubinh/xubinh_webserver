#ifndef __XUBINH_SERVER_UTIL_FILE
#define __XUBINH_SERVER_UTIL_FILE

#include <string>

namespace xubinh_server {

namespace util {

// not thread-safe
//
// - a user-space buffer `FILE *` is used internally
class AppendOnlyPhysicalFile {
public:
    static void set_file_descriptor_availability_polling_time_interval(
        int file_descriptor_availability_polling_time_interval
    ) noexcept {
        _file_descriptor_availability_polling_time_interval =
            file_descriptor_availability_polling_time_interval;
    }

    explicit AppendOnlyPhysicalFile(const std::string &base_name) noexcept;

    // no copy
    AppendOnlyPhysicalFile(const AppendOnlyPhysicalFile &) = delete;
    AppendOnlyPhysicalFile &operator=(const AppendOnlyPhysicalFile &) = delete;

    // no move
    AppendOnlyPhysicalFile(AppendOnlyPhysicalFile &&) = delete;
    AppendOnlyPhysicalFile &operator=(AppendOnlyPhysicalFile &&) = delete;

    // also flushes to disk
    ~AppendOnlyPhysicalFile() noexcept {
        ::fclose(_file_ptr);
    }

    void
    write_to_user_space_memory(const char *data, size_t data_size) noexcept;

    void flush_to_disk() noexcept {
        ::fflush(_file_ptr);
    }

    size_t get_total_number_of_bytes_written() noexcept {
        return _total_number_of_bytes_written;
    }

private:
    static int _file_descriptor_availability_polling_time_interval;

    FILE *_file_ptr = nullptr;

    // for replacing the internal default buffer that `_file_ptr` uses
    char _buffer[64 * 1000]; // ~ 64 KB

    size_t _total_number_of_bytes_written{0};
};

} // namespace util

} // namespace xubinh_server

#endif