#ifndef XUBINH_SERVER_UTIL_FILE
#define XUBINH_SERVER_UTIL_FILE

#include <string>

namespace xubinh_server {

namespace util {

// 对单个物理文件的抽象, 并且是线程不安全的
class AppendOnlyPhysicalFile {
private:
    std::FILE *_file_ptr;
    char _buffer[64 * 1000]; // 用于替换 `_file_ptr` 默认的缓冲区
    size_t _total_number_of_bytes_written;

public:
    // 一个对象负责维护一个文件, 不允许拷贝
    AppendOnlyPhysicalFile(const AppendOnlyPhysicalFile &) = delete;
    AppendOnlyPhysicalFile &operator=(const AppendOnlyPhysicalFile &) = delete;

    // 可以移动但没必要
    AppendOnlyPhysicalFile(AppendOnlyPhysicalFile &&) = delete;
    AppendOnlyPhysicalFile &operator=(AppendOnlyPhysicalFile &&) = delete;

    // 禁用默认构造函数, 禁用隐式转换
    explicit AppendOnlyPhysicalFile(const std::string &base_name);

    ~AppendOnlyPhysicalFile();

    void append(const char *data, size_t data_size);

    // 因为在用户空间维护了一个缓冲区, 因此需要提供一个 flush 函数
    void flush();

    size_t get_total_number_of_bytes_written() {
        return _total_number_of_bytes_written;
    }
};

} // namespace util

} // namespace xubinh_server

#endif