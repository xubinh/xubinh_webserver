#include <chrono>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include "log_builder.h"
#include "log_collector.h"

xubinh_server::LogCollector::CleanUpHelper logger_clean_up_hook;

double run_spdlog(int number_of_messages) {
    auto logger = spdlog::basic_logger_mt(
        "benchmark_logging.spdlog", "benchmark_logging.spdlog.log"
    );

    logger->set_level(spdlog::level::trace);
    logger->set_pattern("%Y-%m-%d %H:%M:%S.%f | %t | %^%l%$ | %v");

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 1; i <= number_of_messages; i++) {
        logger->trace("This is log message number {}", i);
    }

    auto end = std::chrono::high_resolution_clock::now();

    return static_cast<std::chrono::duration<double>>(end - start).count();
}

double run_xubinh_log_builder(int number_of_messages) {
    xubinh_server::LogCollector::set_base_name(
        "benchmark_logging.xubinh_log_builder"
    );

    xubinh_server::LogCollector::set_if_need_output_directly_to_terminal(false);
    xubinh_server::LogBuilder::set_log_level(xubinh_server::LogLevel::TRACE);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 1; i <= number_of_messages; i++) {
        LOG_TRACE << "This is log message number " << i;
    }

    auto end = std::chrono::high_resolution_clock::now();

    return static_cast<std::chrono::duration<double>>(end - start).count();
}

int main() {
    int number_of_messages = 1000000;
    double elapsed_time_interval;
    int average_logs_per_second;

    printf("Number of messages: %d\n", number_of_messages);

    printf("\n");

    printf("spdlog:\n");
    elapsed_time_interval = run_spdlog(number_of_messages);
    average_logs_per_second =
        static_cast<int>(number_of_messages / elapsed_time_interval);
    printf("Elapsed time interval: %f seconds.\n", elapsed_time_interval);
    printf("Average logs per second: %d\n", average_logs_per_second);

    printf("\n");

    printf("xubinh log builder:\n");
    elapsed_time_interval = run_xubinh_log_builder(number_of_messages);
    average_logs_per_second =
        static_cast<int>(number_of_messages / elapsed_time_interval);
    printf("Elapsed time interval: %f seconds.\n", elapsed_time_interval);
    printf("Average logs per second: %d\n", average_logs_per_second);

    return 0;
}
