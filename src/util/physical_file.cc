#include <cerrno>
#include <cstdio>
#include <stdexcept>

#include "util/errno.h"
#include "util/physical_file.h"

namespace xubinh_server {

namespace util {

AppendOnlyPhysicalFile::AppendOnlyPhysicalFile(const std::string &base_name)
    // `ae` 中的 `e` 表示 `O_CLOEXEC` 标志, 即禁止在创建子进程的时候被复制:
    : _file_ptr(fopen(base_name.c_str(), "ae")),
      _total_number_of_bytes_written(0) {

    // 绝大部分情况下不会报错, 一旦报错说明发生了严重资源问题, 应当立即停止:
    if (_file_ptr == nullptr) {
        auto saved_errno = errno;
        auto error_msg = "Failed to create the file `" + base_name
                         + "`: " + strerror_tl(saved_errno);

        throw std::runtime_error(error_msg);
    }

    // 设置一个更大的缓冲区:
    setbuffer(_file_ptr, _buffer, sizeof _buffer);
}

AppendOnlyPhysicalFile::~AppendOnlyPhysicalFile() {
    fclose(_file_ptr);
}

void AppendOnlyPhysicalFile::append(const char *data, size_t data_size) {
    auto current_number_of_bytes_written =
        fwrite_unlocked(data, sizeof(char), data_size, _file_ptr);

    _total_number_of_bytes_written += current_number_of_bytes_written;

    // 由于日志信息可以容忍丢失, 因此这里仅打印错误信息而不抛出异常:
    if (current_number_of_bytes_written != data_size) {
        auto saved_errno = ferror(_file_ptr);

        fprintf(
            stderr,
            "AppendOnlyPhysicalFile::append failed: %s\n",
            strerror_tl(saved_errno).c_str()
        );
    }
}

void AppendOnlyPhysicalFile::flush() {
    fflush(_file_ptr);
}

} // namespace util

} // namespace xubinh_server