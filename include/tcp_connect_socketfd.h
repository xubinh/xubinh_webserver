#ifndef XUBINH_SERVER_TCP_CONNECT_SOCKETFD
#define XUBINH_SERVER_TCP_CONNECT_SOCKETFD

#include <atomic>

#include "inet_address.h"
#include "pollable_file_descriptor.h"
#include "socketfd.h"
#include "tcp_buffer.h"
#include "util/any.h"
#include "util/time_point.h"

namespace xubinh_server {

class TcpConnectSocketfd
    : public std::enable_shared_from_this<TcpConnectSocketfd>,
      public Socketfd {
public:
    using TcpConnectSocketfdPtr = std::shared_ptr<TcpConnectSocketfd>;

    using MessageCallbackType = std::function<void(
        const TcpConnectSocketfdPtr &tcp_connect_socketfd_ptr,
        MutableSizeTcpBuffer *input_buffer,
        const util::TimePoint &time_point
    )>;

    using WriteCompleteCallbackType =
        std::function<void(const TcpConnectSocketfdPtr &tcp_connect_socketfd_ptr
        )>;

    using CloseCallbackType =
        std::function<void(const TcpConnectSocketfdPtr &tcp_connect_socketfd_ptr
        )>;

    TcpConnectSocketfd(
        int fd,
        EventLoop *event_loop,
        const std::string &id,
        const InetAddress &local_address,
        const InetAddress &remote_address
    );

    ~TcpConnectSocketfd() {
        if (!_is_closed) {
            LOG_ERROR << "tcp connection socketfd object being destroyed "
                         "before closing the connection, id: "
                      << _id;

            _close_event_callback();
        }
    }

    const std::string &get_id() const {
        return _id;
    }

    const InetAddress &get_local_address() const {
        return _local_address;
    }

    const InetAddress &get_remote_address() const {
        return _remote_address;
    }

    void register_message_callback(MessageCallbackType message_callback) {
        _message_callback = std::move(message_callback);
    }

    void register_write_complete_callback(
        WriteCompleteCallbackType write_complete_callback
    ) {
        _write_complete_callback = std::move(write_complete_callback);
    }

    // used by internal framework
    void register_close_callback(CloseCallbackType close_callback) {
        _close_callback = std::move(close_callback);
    }

    // not thread-safe
    void start();

    void shutdown();

    // should be called only inside a worker loop
    void send(const char *data, size_t data_size);

    // thread-safe
    void set_time_stamp(const util::TimePoint &time_stamp) {
        _time_stamp.store(time_stamp, std::memory_order_relaxed);
    }

    // thread-safe
    util::TimePoint get_time_stamp() const {
        return _time_stamp.load(std::memory_order_relaxed);
    }

    // used by user
    util::Any context{};

private:
    void _read_event_callback();

    void _write_event_callback();

    void _close_event_callback();

    void _error_event_callback();

    void _receive_all_data();

    size_t _send_as_many_data(const char *data, size_t data_size);

    // size of space the input buffer should expand each time
    static constexpr const int _RECEIVE_DATA_SIZE = 4096;

    const std::string _id;
    const InetAddress _local_address;
    const InetAddress _remote_address;

    std::atomic<util::TimePoint> _time_stamp;

    MutableSizeTcpBuffer _input_buffer;
    MutableSizeTcpBuffer _output_buffer;

    MessageCallbackType _message_callback;
    WriteCompleteCallbackType _write_complete_callback;
    CloseCallbackType _close_callback;

    bool _is_reading = false;
    bool _is_writing = false;
    bool _is_closed = false;

    PollableFileDescriptor _pollable_file_descriptor;
};

} // namespace xubinh_server

#endif