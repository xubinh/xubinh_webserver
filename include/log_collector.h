#ifndef __XUBINH_SERVER_LOG_COLLECTOR
#define __XUBINH_SERVER_LOG_COLLECTOR

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <vector>

#include "log_buffer.h"
#include "util/thread.h"

namespace xubinh_server {

class LogCollector {
public:
    // not thread-safe
    static void set_base_name(const std::string &path);

    static LogCollector &get_instance();

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

    static std::string _base_name;

    static constexpr std::chrono::seconds::rep
        _COLLECT_LOOP_TIMEOUT_IN_SECONDS = 3;

    static constexpr decltype(BufferVector().size()
    ) _DROP_THRESHOLD_OF_CHUNK_BUFFERS_TO_BE_WRITTEN = 16;

    LogCollector();

    ~LogCollector();

    // collect chunk buffers and write into physical files in the background
    void _background_io_thread_worker_functor();

    void _stop(bool also_need_abort);

    ChunkBufferPtr _current_chunk_buffer_ptr{new LogChunkBuffer};
    ChunkBufferPtr _spare_chunk_buffer_ptr{new LogChunkBuffer};

    BufferVector _fulled_chunk_buffers;

    // for internal state and data structures
    std::mutex _mutex;
    std::condition_variable _cond;

    util::Thread _background_thread;
    std::atomic<bool> _need_stop{false};
};

} // namespace xubinh_server

#endif