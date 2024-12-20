#include <cerrno>
#include <cstdio>
#include <stdexcept>
#include <thread>

#include "util/errno.h"
#include "util/physical_file.h"

namespace xubinh_server {

namespace util {

AppendOnlyPhysicalFile::AppendOnlyPhysicalFile(const std::string &base_name) {
    // polling for available fd
    while (true) {
        _file_ptr =
            ::fopen(base_name.c_str(), "ae"); // `e` flag here means `O_CLOEXEC`

        // break if got one
        if (_file_ptr) {
            break;
        }

        // auto saved_errno = errno;
        // auto error_msg = "Failed to create the file `" + base_name
        //                  + "`: " + strerror_tl(saved_errno);

        // throw std::runtime_error(error_msg);

        // otherwise go to sleep for some time
        std::this_thread::sleep_for(
            std::chrono::milliseconds(_file_descriptor_poll_time_interval)
        );
    }

    // replace the default internal buffer with a bigger one
    ::setbuffer(_file_ptr, _buffer, sizeof _buffer);
}

AppendOnlyPhysicalFile::~AppendOnlyPhysicalFile() {
    ::fclose(_file_ptr);
}

void AppendOnlyPhysicalFile::append(const char *data, size_t data_size) {
    auto current_number_of_bytes_written =
        ::fwrite_unlocked(data, sizeof(char), data_size, _file_ptr);

    _total_number_of_bytes_written += current_number_of_bytes_written;

    if (current_number_of_bytes_written != data_size) {
        auto saved_errno = ::ferror(_file_ptr);

        ::fprintf(
            stderr,
            "AppendOnlyPhysicalFile::append failed, reason: %s\n",
            strerror_tl(saved_errno).c_str()
        );
    }
}

void AppendOnlyPhysicalFile::flush() {
    ::fflush(_file_ptr);
}

int AppendOnlyPhysicalFile::_file_descriptor_poll_time_interval = 1500;

} // namespace util

} // namespace xubinh_server