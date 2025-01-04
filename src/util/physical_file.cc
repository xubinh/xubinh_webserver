#include <cerrno>
#include <cstdio>
#include <stdexcept>
#include <thread>

#include "util/errno.h"
#include "util/physical_file.h"

namespace xubinh_server {

namespace util {

AppendOnlyPhysicalFile::AppendOnlyPhysicalFile(const std::string &base_name
) noexcept {
    // polling for available fd
    while (true) {
        _file_ptr =
            ::fopen(base_name.c_str(), "ae"); // `e` flag here means `O_CLOEXEC`

        // exit if got one
        if (_file_ptr) {
            break;
        }

        // otherwise go to sleep for some time
        // [TODO]: optimization required
        std::this_thread::sleep_for(std::chrono::milliseconds(
            _file_descriptor_availability_polling_time_interval
        ));
    }

    // replace the default internal buffer with a bigger one
    ::setbuffer(_file_ptr, _buffer, sizeof _buffer);
}

void AppendOnlyPhysicalFile::write_to_user_space_memory(
    const char *data, size_t data_size
) noexcept {
    auto current_number_of_bytes_written =
        ::fwrite_unlocked(data, sizeof(char), data_size, _file_ptr);

    _total_number_of_bytes_written += current_number_of_bytes_written;

    if (current_number_of_bytes_written != data_size) {
        auto saved_errno = ::ferror(_file_ptr);

        ::fprintf(
            stderr,
            "@xubinh_server::util::AppendOnlyPhysicalFile::write_to_user_space_"
            "memory: %s (errno=%d)\n",
            strerror_tl(saved_errno).c_str(),
            saved_errno
        );
    }
}

int AppendOnlyPhysicalFile::
    _file_descriptor_availability_polling_time_interval = 1500;

} // namespace util

} // namespace xubinh_server