#ifndef XUBINH_SERVER_LOG_COLLECTOR
#define XUBINH_SERVER_LOG_COLLECTOR

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <vector>

#include "log_buffer.h"
#include "utils/thread.h"

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

    // 为内部的缓冲区和阻塞队列提供保护
    std::mutex _mutex;

    // 在前台后台两个线程之间关于阻塞队列建立同步关系
    std::condition_variable _cond;

    utils::Thread _background_thread;
    bool _need_stop = 0;

    LogCollector();

    ~LogCollector();

    void _collect_chunk_buffers_and_write_into_files_in_the_background();

    static constexpr std::chrono::seconds::rep
        _COLLECT_LOOP_TIMEOUT_IN_SECONDS = 3;

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