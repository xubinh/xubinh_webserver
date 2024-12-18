#ifndef __XUBINH_SERVER_PRECONNECT_SOCKETFD
#define __XUBINH_SERVER_PRECONNECT_SOCKETFD

#include "inet_address.h"
#include "log_builder.h"
#include "pollable_file_descriptor.h"
#include "socketfd.h"
#include "util/time_point.h"

namespace xubinh_server {

// not-thread-safe
class PreconnectSocketfd
    : public std::enable_shared_from_this<PreconnectSocketfd>,
      public Socketfd {
public:
    using PreconnectSocketfdPtr = std::shared_ptr<PreconnectSocketfd>;

    using NewConnectionCallbackType = std::function<void(int connect_socketfd)>;

    using ConnectFailCallbackType = std::function<void()>;

    PreconnectSocketfd(
        EventLoop *event_loop,
        const InetAddress &server_address,
        int max_number_of_retries
    );

    // should not modify the listening state of the underlying fd since the
    // ownership will be transfered later to the newly established connection
    ~PreconnectSocketfd() {
        LOG_DEBUG << "preconnect socketfd destructed";
    };

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

    void start();

    // cancel all retries and close immediately
    void cancel();

private:
    // simple wrapper for `::connect` with return values unchanged
    static int
    _connect_to_server(int socketfd, const InetAddress &server_address);

    void _schedule_retry();

    void _write_event_callback();

    // must be guarded by shared_from_this()
    void _try_once();

    static const util::TimeInterval _INITIAL_RETRY_TIME_INTERVAL;

    static const util::TimeInterval _MAX_RETRY_TIME_INTERVAL;

    EventLoop *_event_loop;

    const int _MAX_NUMBER_OF_RETRIES;

    int _number_of_retries{0};

    util::TimeInterval _current_retry_time_interval{
        _INITIAL_RETRY_TIME_INTERVAL};

    std::unique_ptr<TimerIdentifier> _timer_identifier_ptr;

    const InetAddress _server_address;

    NewConnectionCallbackType _new_connection_callback;

    ConnectFailCallbackType _connect_fail_callback;

    PollableFileDescriptor _pollable_file_descriptor;
};

} // namespace xubinh_server

#endif