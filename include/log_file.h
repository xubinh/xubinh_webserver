#ifndef __XUBINH_SERVER_LOG_FILE
#define __XUBINH_SERVER_LOG_FILE

#include <cstdio>
#include <ctime>
#include <memory>
#include <string>

#include "util/physical_file.h"

namespace xubinh_server {

// abstraction of a infinite physical file stream
class LogFile {
public:
    explicit LogFile(const std::string &base_name);

    ~LogFile() noexcept = default;
    // ~LogFile() noexcept {
    //     ::fprintf(stderr, "exit destructor of LogFile\n");
    // };

    void
    write_to_user_space_memory(const char *data, size_t data_size) noexcept {
        _physical_file_ptr->write_to_user_space_memory(data, data_size);

        _do_quick_check();
    }

    void flush_to_disk() noexcept {
        _physical_file_ptr->flush_to_disk();
    }

private:
    bool _is_in_a_new_switch_interval(time_t time_from_epoch_in_seconds
    ) noexcept {
        return (time_from_epoch_in_seconds / _SWITCH_INTERVAL_IN_SECONDS)
               != (_time_of_last_major_check_from_epoch_in_seconds
                   / _SWITCH_INTERVAL_IN_SECONDS);
    }

    std::string _get_name_for_a_new_underlying_physical_file() noexcept;

    void _switch_to_a_new_underlying_physical_file() noexcept {
        auto new_file_name = _get_name_for_a_new_underlying_physical_file();

        _physical_file_ptr.reset(new util::AppendOnlyPhysicalFile(new_file_name)
        );
    }

    // check if need switching by file size
    void _do_quick_check() noexcept;

    // check if need switching by time interval
    void _do_formal_check() noexcept;

    static constexpr size_t _PHYSICAL_FILE_SIZE_THRESHOLD =
        500 * 1000 * 1000; // ~ 500 MB

    static constexpr time_t _FLUSH_INTERVAL_IN_SECONDS = 3; // 3 sec

    static constexpr time_t _SWITCH_INTERVAL_IN_SECONDS = 24 * 60 * 60; // 1 day

    static constexpr int _MAJOR_CHECK_FREQUENCY = 1000;

    const std::string _base_name;

    std::unique_ptr<util::AppendOnlyPhysicalFile> _physical_file_ptr;

    time_t _time_of_last_major_check_from_epoch_in_seconds = 0;

    int _number_of_normal_checks_since_last_major_check = 0;
};

} // namespace xubinh_server

#endif