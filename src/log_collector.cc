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

LogCollector &LogCollector::get_instance() {
    static LogCollector *instance = []() -> LogCollector * {
        // released by CleanUpHelper instance
        _log_collector_singleton_instance = new LogCollector;

        _is_instantiated.store(true, std::memory_order_relaxed);

        return _log_collector_singleton_instance;
    }();

    return *instance;
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
            ::write(STDOUT_FILENO, entry_address, entry_size);

        if (bytes_written != entry_size) {
            ::fprintf(stderr, "error when try to print log into stdout\n");

            ::abort();
        }

        return;
    }

    {
        std::lock_guard<decltype(_mutex)> lock(_mutex);

        // throw away if the entire buffer couldn't hold this single log
        if (entry_size >= _current_chunk_buffer_ptr->capacity()) {
            return;
        }

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

    // ::fprintf(stderr, "exit destructor of LogCollector\n");
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

    BufferVector chunk_buffers_to_be_written;

    LogFile log_file(LogCollector::_base_name);

    std::unique_lock<decltype(_mutex)> lock_hook;

    while (true) {
        {
            std::unique_lock<decltype(_mutex)> lock(_mutex);

            // waits for a few seconds if no fulled buffers come in
            if (_fulled_chunk_buffers.empty()) {
                // spurious wakeup doesn't matter here
                _cond.wait_for(
                    lock, std::chrono::seconds(_COLLECT_LOOP_TIMEOUT_IN_SECONDS)
                );
            }

            // if still no fulled buffers
            if (_fulled_chunk_buffers.empty()) {
                // exits if it's because the outside is telling us to stop
                if (_need_stop.load(std::memory_order_relaxed)) {
                    lock_hook = std::move(lock);

                    break;
                }

                bool expected = true;

                // continue waiting if no flags are set
                if (!_need_flush.compare_exchange_strong(
                        expected, false, std::memory_order_relaxed
                    )) {

                    continue;
                }

                // otherwise creates a dummy fulled-buffer in order to flush the
                // non-fulled one, if the outside asked explicitly (which may
                // happen when the process is about to finish yet blocked due to
                // some bug which in turn requires the log stucking in here to
                // get debugged)
                _spare_chunk_buffer_ptr->append("flush\n", 6);
                _fulled_chunk_buffers.push_back(
                    std::move(_spare_chunk_buffer_ptr)
                );
            }

            _fulled_chunk_buffers.push_back(std::move(_current_chunk_buffer_ptr)
            );

            _fulled_chunk_buffers.swap(chunk_buffers_to_be_written);

            _current_chunk_buffer_ptr =
                std::move(spare_chunk_buffer_for_current_chunk_buffer);

            if (!_spare_chunk_buffer_ptr) {
                _spare_chunk_buffer_ptr =
                    std::move(spare_chunk_buffer_for_spare_chunk_buffer);
            }
        }

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
                                  util::Datetime::Purpose::PRINTING
                              )
                              + "\n";

            log_file.write_to_user_space_memory(msg.c_str(), msg.length());

            chunk_buffers_to_be_written.resize(
                _DROP_THRESHOLD_OF_CHUNK_BUFFERS_TO_BE_WRITTEN
            );
        }

        for (const auto &chunk_buffer_ptr : chunk_buffers_to_be_written) {
            log_file.write_to_user_space_memory(
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

        log_file.flush_to_disk();
    }

    if (_current_chunk_buffer_ptr->length()) {
        log_file.write_to_user_space_memory(
            _current_chunk_buffer_ptr->get_start_address_of_buffer(),
            _current_chunk_buffer_ptr->length()
        );
    }

    // ::fprintf(stderr, "exit LogCollector background thread\n");

    return;
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
        // ::fprintf(stderr, "also need abort\n");

        ::abort();
    }

    // else {
    //     ::fprintf(stderr, "does not need abort\n");
    // }
}

LogCollector *LogCollector::_log_collector_singleton_instance = nullptr;

bool LogCollector::_need_output_directly_to_terminal = false;

std::string LogCollector::_base_name = "log_collector";

constexpr std::chrono::seconds::rep
    LogCollector::_COLLECT_LOOP_TIMEOUT_IN_SECONDS;

std::atomic<bool> LogCollector::_is_instantiated{false};

} // namespace xubinh_server