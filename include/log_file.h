#ifndef XUBINH_SERVER_LOG_FILE
#define XUBINH_SERVER_LOG_FILE

#include <ctime>
#include <memory>
#include <string>

#include "utils/physical_file.h"

namespace xubinh_server {

class utils::AppendOnlyPhysicalFile;

class LogFile {
private:
    const std::string _base_name;
    std::unique_ptr<utils::AppendOnlyPhysicalFile> _physical_file_ptr;
    std::time_t _time_of_last_major_check_from_epoch_in_seconds = 0;
    int _number_of_normal_checks_since_last_major_check = 0;

    std::string _get_name_for_a_new_physical_file();

    void _switch_to_a_new_physical_file();

    void _do_major_check();

    void _do_normal_check();

    // 一个日志文件的大致的大小上限
    static constexpr size_t _PHYSICAL_FILE_SIZE_THRESHOLD = 500 * 1000 * 1000;

    // 刷新底层文件的缓冲区的时间间隔
    static constexpr std::time_t _FLUSH_INTERVAL_IN_SECONDS = 3;

    // 强制切换新文件的时间间隔
    static constexpr std::time_t _SWITCH_INTERVAL_IN_SECONDS = 24 * 60 * 60;

    // 检查操作的执行频率
    static constexpr int _MAJOR_CHECK_FREQUENCY = 1024;

    static inline bool _is_in_a_new_switch_interval(
        time_t time_from_epoch_in_seconds_1, time_t time_from_epoch_in_seconds_2
    ) {
        return (time_from_epoch_in_seconds_1 / _SWITCH_INTERVAL_IN_SECONDS)
               != (time_from_epoch_in_seconds_2 / _SWITCH_INTERVAL_IN_SECONDS);
    }

public:
    explicit LogFile(const std::string &base_name);

    ~LogFile();

    void write(const char *data, size_t data_size);

    void flush();
};

} // namespace xubinh_server

#endif