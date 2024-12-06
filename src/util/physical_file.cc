#include <cerrno>
#include <cstdio>
#include <stdexcept>

#include "util/errno.h"
#include "util/physical_file.h"

namespace xubinh_server {

namespace util {

AppendOnlyPhysicalFile::AppendOnlyPhysicalFile(const std::string &base_name)
    // `e` flag here means `O_CLOEXEC`
    : _file_ptr(fopen(base_name.c_str(), "ae")),
      _total_number_of_bytes_written(0) {

    if (_file_ptr == nullptr) {
        auto saved_errno = errno;
        auto error_msg = "Failed to create the file `" + base_name
                         + "`: " + strerror_tl(saved_errno);

        throw std::runtime_error(error_msg);
    }

    // replace the default internal buffer with a bigger one
    setbuffer(_file_ptr, _buffer, sizeof _buffer);
}

AppendOnlyPhysicalFile::~AppendOnlyPhysicalFile() {
    fclose(_file_ptr);
}

void AppendOnlyPhysicalFile::append(const char *data, size_t data_size) {
    auto current_number_of_bytes_written =
        fwrite_unlocked(data, sizeof(char), data_size, _file_ptr);

    _total_number_of_bytes_written += current_number_of_bytes_written;

    if (current_number_of_bytes_written != data_size) {
        auto saved_errno = ferror(_file_ptr);

        fprintf(
            stderr,
            "AppendOnlyPhysicalFile::append failed, reason: %s\n",
            strerror_tl(saved_errno).c_str()
        );
    }
}

void AppendOnlyPhysicalFile::flush() {
    fflush(_file_ptr);
}

} // namespace util

} // namespace xubinh_server