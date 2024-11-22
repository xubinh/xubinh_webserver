#ifndef XUBINH_SERVER_LOG_COLLECTOR
#define XUBINH_SERVER_LOG_COLLECTOR

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <vector>

#include "log_buffer.h"

namespace xubinh_server {

class LogCollector {
private:
    using ChunkBufferPtr = std::unique_ptr<LogChunkBuffer>;
    using BufferVector = std::vector<ChunkBufferPtr>;

    const std::string _base_name;

    ChunkBufferPtr _current_chunk_buffer_ptr =
        std::make_unique<ChunkBufferPtr::element_type>();
    ChunkBufferPtr _spare_chunk_buffer_ptr =
        std::make_unique<ChunkBufferPtr::element_type>();
    BufferVector _fulled_chunk_buffers;

    //  mutex for writing and moving buffers
    std::mutex _mutex;

    //  condition variable for mutex for writing and moving buffers
    std::condition_variable _cond;

    LogCollector();

    ~LogCollector();

    void _collect_chunk_buffers_and_write_into_files_in_the_background();

    static constexpr std::chrono::seconds::rep _COLLECT_INTERVAL_IN_SECONDS = 3;

    static constexpr decltype(BufferVector().size()
    ) _DROP_THRESHOLD_OF_CHUNK_BUFFERS_TO_BE_WRITTEN = 16;

public:
    LogCollector(const LogCollector &) = delete;
    LogCollector &operator=(const LogCollector &) = delete;

    LogCollector(LogCollector &&) = delete;
    LogCollector &operator=(LogCollector &&) = delete;

    void take_this_log(const char *data, std::size_t data_size);

    static LogCollector &get_instance();
};

} // namespace xubinh_server

#endif