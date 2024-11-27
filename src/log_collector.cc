#include <cassert>

#include "log_collector.h"
#include "log_file.h"
#include "util/datetime.h"
#include "util/format.h"

namespace xubinh_server {

LogCollector::LogCollector()
    : _background_thread(
        std::move(std::bind(
            _collect_chunk_buffers_and_write_into_files_in_the_background, this
        )),
        "logging"
    ) {

    _background_thread.start();
}

LogCollector::~LogCollector() {
    {
        std::lock_guard<std::mutex> lock(_mutex);

        _need_stop = 1;
    }

    _background_thread.join();
}

void LogCollector::
    _collect_chunk_buffers_and_write_into_files_in_the_background() {
    ChunkBufferPtr spare_chunk_buffer_for_current_chunk_buffer(
        new LogChunkBuffer
    );
    ChunkBufferPtr spare_chunk_buffer_for_spare_chunk_buffer(new LogChunkBuffer
    );

    BufferVector chunk_buffers_to_be_written;

    LogFile log_file(LogCollector::_base_name);

    std::unique_lock<decltype(_mutex)> lock_hook;

    while (1) {
        {
            std::unique_lock<decltype(_mutex)> lock(_mutex);

            // 如果还未积累足够多的日志:
            if (_fulled_chunk_buffers.empty()) {
                // 暂时等待一段时间:
                _cond.wait_for(
                    lock, std::chrono::seconds(_COLLECT_LOOP_TIMEOUT_IN_SECONDS)
                );
            }

            // 如果仍然没有积累足够多的日志:
            if (_fulled_chunk_buffers.empty()) {
                // 如果是因为日志系统即将退出:
                if (_need_stop) {
                    // 退出循环并进行善后处理:
                    lock_hook = std::move(lock);

                    break;
                }

                // 否则继续等待:
                else {
                    continue;
                }
            }

            assert(
                !_fulled_chunk_buffers.empty()
                && _current_chunk_buffer_ptr->length() > 0
            );

            _fulled_chunk_buffers.push_back(std::move(_current_chunk_buffer_ptr)
            );

            assert(chunk_buffers_to_be_written.empty());

            _fulled_chunk_buffers.swap(chunk_buffers_to_be_written);

            _current_chunk_buffer_ptr =
                std::move(spare_chunk_buffer_for_current_chunk_buffer);

            if (!_spare_chunk_buffer_ptr) {
                _spare_chunk_buffer_ptr =
                    std::move(spare_chunk_buffer_for_spare_chunk_buffer);
            }
        }

        assert(chunk_buffers_to_be_written.size() >= 2);

        if (chunk_buffers_to_be_written.size()
            > _DROP_THRESHOLD_OF_CHUNK_BUFFERS_TO_BE_WRITTEN) {
            auto number_of_dropped_buffers = static_cast<int>(
                chunk_buffers_to_be_written.size()
                - _DROP_THRESHOLD_OF_CHUNK_BUFFERS_TO_BE_WRITTEN
            );

            std::string msg = "Dropped "
                              + std::to_string(number_of_dropped_buffers)
                              + " chunk buffers at "
                              + util::Datetime::get_datetime_string(
                                  util::DatetimePurpose::PRINTING
                              )
                              + "\n";

            log_file.write(msg.c_str(), msg.length());

            chunk_buffers_to_be_written.resize(
                _DROP_THRESHOLD_OF_CHUNK_BUFFERS_TO_BE_WRITTEN
            );
        }

        assert(
            chunk_buffers_to_be_written.size()
            <= _DROP_THRESHOLD_OF_CHUNK_BUFFERS_TO_BE_WRITTEN
        );

        for (const auto &chunk_buffer_ptr : chunk_buffers_to_be_written) {
            log_file.write(
                chunk_buffer_ptr->get_start_address_of_buffer(),
                chunk_buffer_ptr->length()
            );
        }

        chunk_buffers_to_be_written.resize(2);

        if (!spare_chunk_buffer_for_current_chunk_buffer) {
            spare_chunk_buffer_for_current_chunk_buffer =
                std::move(chunk_buffers_to_be_written[0]);
            spare_chunk_buffer_for_current_chunk_buffer->reset();
        }

        if (!spare_chunk_buffer_for_spare_chunk_buffer) {
            spare_chunk_buffer_for_spare_chunk_buffer =
                std::move(chunk_buffers_to_be_written[1]);
            spare_chunk_buffer_for_spare_chunk_buffer->reset();
        }

        chunk_buffers_to_be_written.clear();

        log_file.flush();
    }

    assert(_need_stop && _fulled_chunk_buffers.empty());

    if (_current_chunk_buffer_ptr->length()) {
        log_file.write(
            _current_chunk_buffer_ptr->get_start_address_of_buffer(),
            _current_chunk_buffer_ptr->length()
        );
    }

    return;
}

std::string LogCollector::_base_name = "log_collector";

void LogCollector::take_this_log(const char *entry_address, size_t entry_size) {
    std::lock_guard<decltype(_mutex)> lock(_mutex);

    // 如果外部传进来的单条日志超过了内部缓冲区的额定大小则直接丢弃
    // (一般不会发生, 因为外部缓冲区总是应该比内部缓冲区来得小):
    if (entry_size >= _current_chunk_buffer_ptr->capacity()) {
        return;
    }

    // 如果当前缓冲区还能够容纳得下本条日志:
    if (entry_size < _current_chunk_buffer_ptr->length_of_spare()) {
        _current_chunk_buffer_ptr->append(entry_address, entry_size);

        return;
    }

    // 否则将当前缓冲区放至阻塞队列中:
    _fulled_chunk_buffers.push_back(std::move(_current_chunk_buffer_ptr));

    // 然后切换备用的内部缓冲区 (如果备用的也用光了那就新建一个):
    _current_chunk_buffer_ptr = _spare_chunk_buffer_ptr
                                    ? std::move(_spare_chunk_buffer_ptr)
                                    : ChunkBufferPtr(new LogChunkBuffer);

    // 写入本条日志:
    _current_chunk_buffer_ptr->append(entry_address, entry_size);

    // 此时必然至少有两个非空的 chunk buffer, 因此需要通知后台的 I/O 线程:
    _cond.notify_all();
}

void LogCollector::set_base_name(const std::string &path) {
    LogCollector::_base_name =
        util::Format::get_base_name_of_path(path.c_str(), path.length());
}

LogCollector &LogCollector::get_instance() {
    static LogCollector instance;

    return instance;
}

} // namespace xubinh_server