#ifndef __XUBINH_SERVER_LOG_FILE
#define __XUBINH_SERVER_LOG_FILE

#include <ctime>
#include <memory>
#include <string>

#include "util/physical_file.h"

namespace xubinh_server {

class LogFile {
public:
    explicit LogFile(const std::string &base_name);

    ~LogFile();

    void write(const char *data, size_t data_size);

    void flush();

private:
    static constexpr size_t _PHYSICAL_FILE_SIZE_THRESHOLD = 500 * 1000 * 1000;

    static constexpr time_t _FLUSH_INTERVAL_IN_SECONDS = 3;

    static constexpr time_t _SWITCH_INTERVAL_IN_SECONDS = 24 * 60 * 60;

    static constexpr int _MAJOR_CHECK_FREQUENCY = 1000;

    static inline bool _is_in_a_new_switch_interval(
        time_t time_from_epoch_in_seconds_1, time_t time_from_epoch_in_seconds_2
    ) {
        return (time_from_epoch_in_seconds_1 / _SWITCH_INTERVAL_IN_SECONDS)
               != (time_from_epoch_in_seconds_2 / _SWITCH_INTERVAL_IN_SECONDS);
    }

    std::string _get_name_for_a_new_physical_file();

    void _switch_to_a_new_physical_file();

    // cost time, called sparingly
    void _do_major_check();

    // called frequently
    void _do_normal_check();

    const std::string _base_name;
    std::unique_ptr<util::AppendOnlyPhysicalFile> _physical_file_ptr;
    time_t _time_of_last_major_check_from_epoch_in_seconds = 0;
    int _number_of_normal_checks_since_last_major_check = 0;
};

} // namespace xubinh_server

#endif