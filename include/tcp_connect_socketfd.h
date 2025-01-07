#ifndef __XUBINH_SERVER_TCP_CONNECT_SOCKETFD
#define __XUBINH_SERVER_TCP_CONNECT_SOCKETFD

#include <atomic>

#include "inet_address.h"
#include "pollable_file_descriptor.h"
#include "socketfd.h"
#include "tcp_buffer.h"
#include "util/any.h"
#include "util/time_point.h"

namespace xubinh_server {

class alignas(64) TcpConnectSocketfd
    : public std::enable_shared_from_this<TcpConnectSocketfd>,
      public Socketfd {
public:
    using TcpConnectSocketfdPtr = std::shared_ptr<TcpConnectSocketfd>;

    using MessageCallbackType = std::function<void(
        TcpConnectSocketfd *tcp_connect_socketfd_ptr,
        MutableSizeTcpBuffer *input_buffer,
        util::TimePoint time_stamp
    )>;

    using WriteCompleteCallbackType = std::function<void(TcpConnectSocketfd *)>;

    using CloseCallbackType =
        std::function<void(TcpConnectSocketfd *tcp_connect_socketfd_ptr)>;

    using PredicateType = std::function<bool()>;

    TcpConnectSocketfd(
        int fd,
        EventLoop *loop,
        const uint64_t &id,
        const InetAddress &local_address,
        const InetAddress &remote_address,
        util::TimePoint time_stamp
    );

    ~TcpConnectSocketfd();

    const uint64_t &get_id() const {
        return _id;
    }

    const InetAddress &get_local_address() const {
        return _local_address;
    }

    const InetAddress &get_remote_address() const {
        return _remote_address;
    }

    void register_message_callback(const MessageCallbackType &message_callback
    ) {
        _message_callback = message_callback;
    }

    void register_message_callback(MessageCallbackType &&message_callback) {
        _message_callback = std::move(message_callback);
    }

    void register_write_complete_callback(
        const WriteCompleteCallbackType &write_complete_callback
    ) {
        _write_complete_callback = write_complete_callback;
    }

    void register_write_complete_callback(
        WriteCompleteCallbackType &&write_complete_callback
    ) {
        _write_complete_callback = std::move(write_complete_callback);
    }

    // used by internal framework
    void register_close_callback(const CloseCallbackType &close_callback) {
        _close_callback = close_callback;
    }

    // used by internal framework
    void register_close_callback(CloseCallbackType &&close_callback) {
        _close_callback = std::move(close_callback);
    }

    // not thread-safe
    void start();

    // used by external user; identical to the internal one, just for
    // convenience here
    bool is_writing() const {
        return _pollable_file_descriptor.is_writing();
    }

    // close local write-end
    //
    // - can be called by user to start the four-way handshake; after which
    // it is the user's responsibility to ensure there will be no data sent to
    // the output buffer
    // - should only be called inside a worker loop
    void shutdown_write();

    bool is_write_end_shutdown() const {
        return _is_write_end_shutdown;
    }

    // abruptly reset the entire connection
    //
    // - can be called by user to reset the connection; after which
    // it is the user's responsibility to ensure there will be no data sent to
    // the output buffer
    // - must be called in the worker loop
    void abort();

    // abruptly reset the entire connection when certain condition is met
    //
    // - can be called by user to reset the connection; after which
    // it is the user's responsibility to ensure there will be no data sent to
    // the output buffer
    // - can be called either in main loop or in worker loop
    void check_and_abort(PredicateType predicate);

    // should only be called inside a worker loop
    void send(const char *data, size_t data_size);

    // thread-safe
    void set_time_stamp(util::TimePoint time_stamp) {
        _time_stamp.store(time_stamp, std::memory_order_relaxed);
    }

    // thread-safe
    util::TimePoint get_time_stamp() const {
        return _time_stamp.load(std::memory_order_relaxed);
    }

    uint64_t get_loop_index() const noexcept;

    // used by user
    util::Any context{};

    void clear_context() {
        context.clear();
    }

private:
    // reads in all available data, let the outside process the data, and
    // possibly sends response out
    void _read_event_callback(util::TimePoint time_stamp);

    // sends out the response that could not be sent in one go inside the read
    // event callback
    void _write_event_callback();

    // detaches from the poller and let the outside release the resources
    void _close_event_callback();

    // only reads and prints the error happened on the socketfd, after which the
    // socketfd will still be at the error state
    void _error_event_callback();

    bool _is_reading() const {
        return _pollable_file_descriptor.is_reading();
    }

    bool _is_writing() const {
        return _pollable_file_descriptor.is_writing();
    }

    bool _is_stopped() const {
        return _pollable_file_descriptor.is_detached();
    }

    void _check_and_abort_impl(PredicateType predicate);

    // only start reading when a read event is encountered
    size_t _receive_all_data();

    // always start writing first and only enable write event when necessary
    //
    // - SIGPIPE is disabled internally
    size_t _send_as_many_data(const char *data, size_t data_size);

    // size of space the input buffer should expand each time
    //
    // - make it half the initial size of the buffer to prevent unnecessary
    // reallocation
    static constexpr const size_t _RECEIVE_DATA_SIZE = 4096; // 4 KB

    char recv_buffer[_RECEIVE_DATA_SIZE];

    const uint64_t _id;
    const InetAddress _local_address;
    const InetAddress _remote_address;

    EventLoop *_loop;

    std::atomic<util::TimePoint> _time_stamp;

    MutableSizeTcpBuffer _input_buffer;
    MutableSizeTcpBuffer
        _output_buffer; // [TODO]: make it a queue of heterogeneous data
                        // sources (e.g. std::string, sendFile, mmap,
                        // etc.) for better performance

    MessageCallbackType _message_callback;
    WriteCompleteCallbackType _write_complete_callback;
    CloseCallbackType _close_callback;

    bool _is_write_end_shutdown = false;
    bool _is_abotrted = false;

    PollableFileDescriptor _pollable_file_descriptor;
};

} // namespace xubinh_server

#endif