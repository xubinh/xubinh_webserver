#ifndef __XUBINH_SERVER_LOG_COLLECTOR
#define __XUBINH_SERVER_LOG_COLLECTOR

#include <atomic>
#include <memory>
#include <vector>

#include "log_buffer.h"
#include "util/condition_variable.h"
#include "util/mutex.h"
#include "util/thread.h"

namespace xubinh_server {

class LogCollector {
public:
    struct CleanUpHelper {
        // must be called exactly once and before the definitions of all global
        // variables that depends on the logger
        CleanUpHelper() = default;

        ~CleanUpHelper() {
            if (_is_instantiated.load(std::memory_order_relaxed)) {
                delete _log_collector_singleton_instance;
            }
        }
    };

    // not thread-safe
    static void set_base_name(const std::string &path);

    // not thread-safe
    //
    // - must be called before first visiting the singleton instance
    static void set_if_need_output_directly_to_terminal(bool yes_or_no) {
        _need_output_directly_to_terminal = yes_or_no;
    }

    static LogCollector &get_instance() {
        static LogCollector *instance = []() -> LogCollector * {
            // released by CleanUpHelper instance
            _log_collector_singleton_instance = new LogCollector;

            _is_instantiated.store(true, std::memory_order_relaxed);

            return _log_collector_singleton_instance;
        }();

        return *instance;
    }

    static void flush();

    // no copy
    LogCollector(const LogCollector &) = delete;
    LogCollector &operator=(const LogCollector &) = delete;

    // no move
    LogCollector(LogCollector &&) = delete;
    LogCollector &operator=(LogCollector &&) = delete;

    // thread-safe
    void take_this_log(const char *data, size_t data_size);

    // thread-safe
    void abort();

private:
    using ChunkBufferPtr = std::unique_ptr<LogChunkBuffer>;
    using BufferVector = std::vector<ChunkBufferPtr>;

    LogCollector();

    ~LogCollector();

    // collect chunk buffers and write into physical files in the background
    void _background_io_thread_worker_functor();

    void _stop(bool also_need_abort);

    static bool _need_output_directly_to_terminal;

    static std::string _base_name;

    static const util::TimeInterval _COLLECT_LOOP_TIME_INTERVAL;

    static constexpr size_t _DROP_THRESHOLD_OF_CHUNK_BUFFERS_TO_BE_WRITTEN = 16;

    static LogCollector *_log_collector_singleton_instance;

    // only for flush
    static std::atomic<bool> _is_instantiated;

    ChunkBufferPtr _current_chunk_buffer_ptr{new LogChunkBuffer};
    ChunkBufferPtr _spare_chunk_buffer_ptr{new LogChunkBuffer};

    BufferVector _fulled_chunk_buffers;

    // for internal state and data structures
    util::Mutex _mutex;
    util::ConditionVariable _cond;

    util::Thread _background_thread;
    std::atomic<bool> _need_flush{false};
    std::atomic<bool> _need_stop{false};
};

} // namespace xubinh_server

#endif