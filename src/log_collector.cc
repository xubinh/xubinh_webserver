#include "log_collector.h"
#include "log_file.h"
#include "util/datetime.h"
#include "util/format.h"

namespace xubinh_server {

void LogCollector::set_base_name(const std::string &path) {
    if (path.empty()) {
        return;
    }

    LogCollector::_base_name =
        util::Format::get_base_name_of_path(path.c_str(), path.length());
}

void LogCollector::flush() {
    if (_is_instantiated.load(std::memory_order_relaxed)) {
        // no need to CAS; multiple flush requests could be merged into one
        get_instance()._need_flush.store(true, std::memory_order_relaxed);
    }
}

void LogCollector::take_this_log(const char *entry_address, size_t entry_size) {
    if (_need_output_directly_to_terminal) {
        ssize_t bytes_written =
            ::write(STDERR_FILENO, entry_address, entry_size);

        if (bytes_written != static_cast<ssize_t>(entry_size)) {
            ::fprintf(
                stderr,
                "@xubinh_server::LogCollector::take_this_log: error when "
                "trying to write log into stderr using `write`\n"
            );

            ::abort();
        }

        return;
    }

    {
        util::MutexGuard lock(_mutex);

        // no buffer replacement if the current buffer has enough space
        if (entry_size < _current_chunk_buffer_ptr->length_of_spare()) {
            _current_chunk_buffer_ptr->append(entry_address, entry_size);

            return;
        }

        _fulled_chunk_buffers.push_back(std::move(_current_chunk_buffer_ptr));

        _current_chunk_buffer_ptr = _spare_chunk_buffer_ptr
                                        ? std::move(_spare_chunk_buffer_ptr)
                                        : ChunkBufferPtr(new LogChunkBuffer);

        _current_chunk_buffer_ptr->append(entry_address, entry_size);
    }

    // must have two non-empty buffers (i.e. the replaced fulled buffer and the
    // current buffer that the log was written in) now, so signal the background
    // I/O thread
    _cond.notify_all();
}

void LogCollector::abort() {
    _stop(true);
}

LogCollector::LogCollector()
    : _background_thread(
        [this]() {
            _background_io_thread_worker_functor();
        },
        "logging"
    ) {

    _background_thread.start();
}

LogCollector::~LogCollector() {
    _stop(false);
}

void LogCollector::_background_io_thread_worker_functor() {
    if (_need_output_directly_to_terminal) {
        return;
    }

    ChunkBufferPtr spare_chunk_buffer_for_current_chunk_buffer(
        new LogChunkBuffer
    );
    ChunkBufferPtr spare_chunk_buffer_for_spare_chunk_buffer(new LogChunkBuffer
    );

    BufferVector fulled_chunk_buffers_to_be_written;

    LogFile log_file(LogCollector::_base_name);

    while (true) {
        {
            util::MutexGuard lock(_mutex);

            if (_fulled_chunk_buffers.empty()) {
                // spurious wakeup is OK; we have the outer while loop applied
                _cond.wait_for(lock, _COLLECT_LOOP_TIME_INTERVAL);
            }

            if (_fulled_chunk_buffers.empty()) {
                if (_need_stop.load(std::memory_order_relaxed)) {
                    if (_current_chunk_buffer_ptr->length()) {
                        log_file.write_to_user_space_memory(
                            _current_chunk_buffer_ptr
                                ->get_start_address_of_buffer(),
                            _current_chunk_buffer_ptr->length()
                        );
                    }

                    return;
                }

                bool expected = true;

                if (!_need_flush.compare_exchange_strong(
                        expected, false, std::memory_order_relaxed
                    )) {

                    continue;
                }

                // for flushing: creates a dummy fulled-buffer in order to flush
                // the non-fulled one, if the outside asked explicitly (which
                // may happen when the process is about to finish yet blocked
                // due to some bug which in turn requires the log stucking in
                // here to get debugged)
                _spare_chunk_buffer_ptr->append("flush\n", 6);
                _fulled_chunk_buffers.push_back(
                    std::move(_spare_chunk_buffer_ptr)
                );
            }

            _fulled_chunk_buffers.push_back(std::move(_current_chunk_buffer_ptr)
            );

            _fulled_chunk_buffers.swap(fulled_chunk_buffers_to_be_written);

            _current_chunk_buffer_ptr =
                std::move(spare_chunk_buffer_for_current_chunk_buffer);

            if (!_spare_chunk_buffer_ptr) {
                _spare_chunk_buffer_ptr =
                    std::move(spare_chunk_buffer_for_spare_chunk_buffer);
            }
        }

        if (fulled_chunk_buffers_to_be_written.size()
            > _DROP_THRESHOLD_OF_CHUNK_BUFFERS_TO_BE_WRITTEN) {

            auto number_of_dropped_buffers = static_cast<int>(
                fulled_chunk_buffers_to_be_written.size()
                - _DROP_THRESHOLD_OF_CHUNK_BUFFERS_TO_BE_WRITTEN
            );

            std::string msg = "Dropped "
                              + std::to_string(number_of_dropped_buffers)
                              + " chunk buffers at "
                              + util::Datetime::get_datetime_string(
                                  util::Datetime::Purpose::PRINTING
                              )
                              + "\n";

            log_file.write_to_user_space_memory(msg.c_str(), msg.length());

            fulled_chunk_buffers_to_be_written.resize(
                _DROP_THRESHOLD_OF_CHUNK_BUFFERS_TO_BE_WRITTEN
            );
        }

        for (const auto &chunk_buffer_ptr :
             fulled_chunk_buffers_to_be_written) {
            log_file.write_to_user_space_memory(
                chunk_buffer_ptr->get_start_address_of_buffer(),
                chunk_buffer_ptr->length()
            );
        }

        fulled_chunk_buffers_to_be_written.resize(2);

        if (!spare_chunk_buffer_for_current_chunk_buffer) {
            spare_chunk_buffer_for_current_chunk_buffer =
                std::move(fulled_chunk_buffers_to_be_written[0]);

            spare_chunk_buffer_for_current_chunk_buffer->reset();
        }

        if (!spare_chunk_buffer_for_spare_chunk_buffer) {
            spare_chunk_buffer_for_spare_chunk_buffer =
                std::move(fulled_chunk_buffers_to_be_written[1]);

            spare_chunk_buffer_for_spare_chunk_buffer->reset();
        }

        fulled_chunk_buffers_to_be_written.clear();

        log_file.flush_to_disk();
    }
}

void LogCollector::_stop(bool also_need_abort) {
    bool expected = false;

    if (!_need_stop.compare_exchange_strong(
            expected, true, std::memory_order_relaxed, std::memory_order_relaxed
        )) {

        return;
    }

    _background_thread.join();

    if (also_need_abort) {
        ::abort();
    }
}

LogCollector *LogCollector::_log_collector_singleton_instance = nullptr;

bool LogCollector::_need_output_directly_to_terminal = false;

std::string LogCollector::_base_name = "log_collector";

const util::TimeInterval LogCollector::_COLLECT_LOOP_TIME_INTERVAL =
    3 * util::TimeInterval::SECOND;

std::atomic<bool> LogCollector::_is_instantiated{false};

} // namespace xubinh_server