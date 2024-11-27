#ifndef XUBINH_SERVER_LOG_COLLECTOR
#define XUBINH_SERVER_LOG_COLLECTOR

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <vector>

#include "log_buffer.h"
#include "util/thread.h"

namespace xubinh_server {

class LogCollector {
private:
    using ChunkBufferPtr = std::unique_ptr<LogChunkBuffer>;
    using BufferVector = std::vector<ChunkBufferPtr>;

    ChunkBufferPtr _current_chunk_buffer_ptr{new LogChunkBuffer};
    ChunkBufferPtr _spare_chunk_buffer_ptr{new LogChunkBuffer};

    BufferVector _fulled_chunk_buffers;

    // 为内部的缓冲区和阻塞队列提供保护
    std::mutex _mutex;

    // 在前台后台两个线程之间关于阻塞队列建立同步关系
    std::condition_variable _cond;

    util::Thread _background_thread;
    bool _need_stop = 0;

    LogCollector();

    ~LogCollector();

    void _collect_chunk_buffers_and_write_into_files_in_the_background();

    static std::string _base_name;

    static constexpr std::chrono::seconds::rep
        _COLLECT_LOOP_TIMEOUT_IN_SECONDS = 3;

    static constexpr decltype(BufferVector().size()
    ) _DROP_THRESHOLD_OF_CHUNK_BUFFERS_TO_BE_WRITTEN = 16;

public:
    LogCollector(const LogCollector &) = delete;
    LogCollector &operator=(const LogCollector &) = delete;

    LogCollector(LogCollector &&) = delete;
    LogCollector &operator=(LogCollector &&) = delete;

    void take_this_log(const char *data, size_t data_size);

    static void set_base_name(const std::string &path);

    static LogCollector &get_instance();
};

} // namespace xubinh_server

#endif