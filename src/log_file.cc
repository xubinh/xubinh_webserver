#include <stdexcept>

#include "log_file.h"
#include "utils/datetime.h"

namespace xubinh_server {

std::string LogFile::_get_name_for_a_new_physical_file() {
    std::string file_name;
    auto datetime_string =
        utils::Datetime::get_datetime_string(utils::DatetimePurpose::RENAMING);

    file_name.reserve(_base_name.length() + 32);

    file_name += _base_name;      // "my_server"
    file_name += ".";             // "my_server."
    file_name += datetime_string; // "my_server.2024_01_01_13_00_23"
    file_name += ".log";          // "my_server.2024_01_01_13_00_23.log"

    return file_name;
}

void LogFile::_switch_to_a_new_physical_file() {
    auto new_file_name = _get_name_for_a_new_physical_file();

    _physical_file_ptr.reset(new utils::AppendOnlyPhysicalFile(new_file_name));
}

void LogFile::_do_major_check() {
    auto current_time_from_epoch_in_seconds =
        utils::Datetime::get_current_time_from_epoch_in_seconds();

    // 如果进入新的物理文件切换周期:
    if (_is_in_a_new_switch_interval(
            _time_of_last_major_check_from_epoch_in_seconds,
            current_time_from_epoch_in_seconds
        )) {
        // 强制切换文件:
        _switch_to_a_new_physical_file();
    }

    // 如果两次 major check 之间的间隔大于缓冲区刷新阈值:
    else if (current_time_from_epoch_in_seconds - _time_of_last_major_check_from_epoch_in_seconds > _FLUSH_INTERVAL_IN_SECONDS) {
        // 刷新缓冲区:
        flush();
    }

    // 因为上述操作可能会比较耗时, 因此索性重新获取一次时间戳:
    _time_of_last_major_check_from_epoch_in_seconds =
        utils::Datetime::get_current_time_from_epoch_in_seconds();
}

void LogFile::_do_normal_check() {
    _number_of_normal_checks_since_last_major_check++;

    // 如果物理文件大小超出阈值:
    if (_physical_file_ptr->get_total_number_of_bytes_written()
        > _PHYSICAL_FILE_SIZE_THRESHOLD) {
        // 切换新文件:
        _switch_to_a_new_physical_file();

        // 由于 major check 就是为了切换新文件, 因此可以将其推迟:
        _number_of_normal_checks_since_last_major_check = 0;

        return;
    }

    // 如果还不需要进行 major check 则直接返回:
    if (_number_of_normal_checks_since_last_major_check
        <= _MAJOR_CHECK_FREQUENCY) {
        return;
    }

    _do_major_check();

    _number_of_normal_checks_since_last_major_check = 0;
}

LogFile::LogFile(const std::string &base_name) : _base_name(base_name) {
    // 如果文件路径包含斜杠, 有可能会造成安全隐患, 因此需要抛出异常:
    if (base_name.find('/') != std::string::npos) {
        throw std::runtime_error("File name contains slash which is unsafe");
    }

    _switch_to_a_new_physical_file();
}

LogFile::~LogFile() = default;

void LogFile::write(const char *data, size_t data_size) {
    _physical_file_ptr->append(data, data_size);

    _do_normal_check();
}

void LogFile::flush() {
    _physical_file_ptr->flush();
}

} // namespace xubinh_server