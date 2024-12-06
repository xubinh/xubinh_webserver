#include <stdexcept>

#include "log_file.h"
#include "util/datetime.h"

namespace xubinh_server {

LogFile::LogFile(const std::string &base_name) : _base_name(base_name) {
    // slashes could cause folders to be created and should be avoided for
    // security reason
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

std::string LogFile::_get_name_for_a_new_physical_file() {
    std::string file_name;
    auto datetime_string =
        util::Datetime::get_datetime_string(util::DatetimePurpose::RENAMING);

    file_name.reserve(_base_name.length() + 32);

    file_name += _base_name;      // "my_server"
    file_name += ".";             // "my_server."
    file_name += datetime_string; // "my_server.2024_01_01_13_00_23"
    file_name += ".log";          // "my_server.2024_01_01_13_00_23.log"

    return file_name;
}

void LogFile::_switch_to_a_new_physical_file() {
    auto new_file_name = _get_name_for_a_new_physical_file();

    _physical_file_ptr.reset(new util::AppendOnlyPhysicalFile(new_file_name));
}

void LogFile::_do_major_check() {
    auto current_time_from_epoch_in_seconds =
        util::Datetime::get_current_time_from_epoch_in_seconds();

    // forcibly switch
    if (_is_in_a_new_switch_interval(
            _time_of_last_major_check_from_epoch_in_seconds,
            current_time_from_epoch_in_seconds
        )) {

        _switch_to_a_new_physical_file();
    }

    // or manually flush
    else if (current_time_from_epoch_in_seconds - _time_of_last_major_check_from_epoch_in_seconds > _FLUSH_INTERVAL_IN_SECONDS) {
        flush();
    }

    // simply refetch the time again
    _time_of_last_major_check_from_epoch_in_seconds =
        util::Datetime::get_current_time_from_epoch_in_seconds();
}

void LogFile::_do_normal_check() {
    _number_of_normal_checks_since_last_major_check += 1;

    if (_physical_file_ptr->get_total_number_of_bytes_written()
        > _PHYSICAL_FILE_SIZE_THRESHOLD) {

        _switch_to_a_new_physical_file();

        // clear the count down since the file is already switched
        _number_of_normal_checks_since_last_major_check = 0;

        return;
    }

    if (_number_of_normal_checks_since_last_major_check
        <= _MAJOR_CHECK_FREQUENCY) {
        return;
    }

    _do_major_check();

    _number_of_normal_checks_since_last_major_check = 0;
}

} // namespace xubinh_server