#include <stdexcept>

#include "log_file.h"
#include "util/datetime.h"

namespace xubinh_server {

LogFile::LogFile(const std::string &base_name)
    : _base_name(base_name) {
    // slashes could cause folders to be created and should be avoided for
    // security reason
    if (base_name.find('/') != std::string::npos) {
        throw std::runtime_error("File name contains slash which is unsafe");
    }

    _switch_to_a_new_underlying_physical_file();
}

std::string LogFile::_get_name_for_a_new_underlying_physical_file() noexcept {
    std::string file_name;

    auto datetime_string =
        util::Datetime::get_datetime_string(util::Datetime::Purpose::RENAMING);

    file_name.reserve(_base_name.length() + 32);

    file_name += _base_name;      // "my_server"
    file_name += ".";             // "my_server."
    file_name += datetime_string; // "my_server.2024_01_01_13_00_23"
    file_name += ".log";          // "my_server.2024_01_01_13_00_23.log"

    return file_name;
}

void LogFile::_do_quick_check() noexcept {
    _number_of_normal_checks_since_last_major_check += 1;

    if (_physical_file_ptr->get_total_number_of_bytes_written()
        > _PHYSICAL_FILE_SIZE_THRESHOLD) {

        _switch_to_a_new_underlying_physical_file();

        // clear the count down since the file is already switched
        _number_of_normal_checks_since_last_major_check = 0;

        return;
    }

    if (_number_of_normal_checks_since_last_major_check
        <= _MAJOR_CHECK_FREQUENCY) {
        return;
    }

    _do_formal_check();

    _number_of_normal_checks_since_last_major_check = 0;
}

void LogFile::_do_formal_check() noexcept {
    auto current_time_from_epoch_in_seconds =
        util::Datetime::get_current_time_from_epoch_in_seconds();

    // forcibly switch by big interval
    if (_is_in_a_new_switch_interval(current_time_from_epoch_in_seconds)) {
        _switch_to_a_new_underlying_physical_file();

        // refetch time to lower the above I/O operation's frequency
        _time_of_last_major_check_from_epoch_in_seconds =
            util::Datetime::get_current_time_from_epoch_in_seconds();
    }

    // manually flush by small interval
    else if (current_time_from_epoch_in_seconds - _time_of_last_major_check_from_epoch_in_seconds > _FLUSH_INTERVAL_IN_SECONDS) {
        flush_to_disk();

        // refetch time to lower the above I/O operation's frequency
        _time_of_last_major_check_from_epoch_in_seconds =
            util::Datetime::get_current_time_from_epoch_in_seconds();
    }

    else {
        // simply reuse the time if no I/O operation is made
        _time_of_last_major_check_from_epoch_in_seconds =
            current_time_from_epoch_in_seconds;
    }
}

} // namespace xubinh_server