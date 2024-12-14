#ifndef XUBINH_SERVER_PRECONNECT_SOCKETFD
#define XUBINH_SERVER_PRECONNECT_SOCKETFD

#include "inet_address.h"
#include "log_builder.h"
#include "pollable_file_descriptor.h"
#include "socketfd.h"
#include "util/time_point.h"

namespace xubinh_server {

class PreconnectSocketfd
    : public std::enable_shared_from_this<PreconnectSocketfd>,
      public Socketfd {
public:
    using PreconnectSocketfdPtr = std::shared_ptr<PreconnectSocketfd>;

    using NewConnectionCallbackType = std::function<void(
        const PreconnectSocketfdPtr &preconnect_socketfd_ptr,
        int connect_socketfd
    )>;

    using ConnectFailCallbackType =
        std::function<void(const PreconnectSocketfdPtr &preconnect_socketfd_ptr
        )>;

    PreconnectSocketfd(
        int fd,
        EventLoop *event_loop,
        const InetAddress &server_address,
        int max_number_of_retries
    );

    ~PreconnectSocketfd() {
        _pollable_file_descriptor.disable_write_event();
    }

    void register_new_connection_callback(
        NewConnectionCallbackType new_connection_callback
    ) {
        _new_connection_callback = std::move(new_connection_callback);
    }

    void
    register_connect_fail_callback(ConnectFailCallbackType connect_fail_callback
    ) {
        _connect_fail_callback = std::move(connect_fail_callback);
    }

    void start() {
        if (!_new_connection_callback) {
            LOG_FATAL << "missing new connection callback";
        }

        _event_loop->run(std::bind(_try_once, shared_from_this()));
    }

private:
    // simple wrapper for `::connect` with return values unchanged
    static int
    _connect_to_server(int socketfd, const InetAddress &server_address);

    static constexpr const util::TimeInterval _INITIAL_RETRY_TIME_INTERVAL{
        static_cast<uint64_t>(500 * 1000 * 1000)}; // 0.5 sec

    static constexpr const util::TimeInterval _MAX_RETRY_TIME_INTERVAL{
        static_cast<uint64_t>(30 * 1000 * 1000 * 1000)}; // 30 sec

    void _schedule_retry();

    void _write_event_callback();

    // must be guarded by shared_from_this()
    void _try_once();

    EventLoop *_event_loop;

    const int _MAX_NUMBER_OF_RETRIES;

    int _number_of_retries{0};

    util::TimeInterval _current_retry_time_interval{
        _INITIAL_RETRY_TIME_INTERVAL};

    const InetAddress _server_address;

    NewConnectionCallbackType _new_connection_callback;

    ConnectFailCallbackType _connect_fail_callback;

    PollableFileDescriptor _pollable_file_descriptor;
};

} // namespace xubinh_server

#endif