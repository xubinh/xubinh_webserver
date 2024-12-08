#ifndef XUBINH_SERVER_EVENT_POLLER
#define XUBINH_SERVER_EVENT_POLLER

#include <sys/epoll.h>
#include <unordered_set>
#include <vector>

#include "pollable_file_descriptor.h"
#include "util/type_traits.h"

namespace xubinh_server {

// not thread-safe
class EventPoller {
private:
    using EventTypeEnumUnderlyingType =
        util::type_traits::underlying_type_t<decltype(EPOLLIN)>;

public:
    enum EventType : EventTypeEnumUnderlyingType {
        EVENT_TYPE_READ =
            EPOLLIN | EPOLLPRI | EPOLLRDHUP, // even for non-socket fds
        EVENT_TYPE_WRITE = EPOLLOUT,
        EVENT_TYPE_CLOSE = EPOLLHUP,
        EVENT_TYPE_ERROR = EPOLLERR,
        EVENT_TYPE_ALL = EVENT_TYPE_READ | EVENT_TYPE_WRITE
    };

    EventPoller();

    ~EventPoller();

    void register_event_for_fd(int fd, const epoll_event *event);

    std::vector<PollableFileDescriptor *> poll_for_active_events_of_all_fds();

private:
    static constexpr int _MAX_SIZE_OF_EVENT_ARRAY = 65536;

    // - ignored but still must be greater than zero since Linux 2.6.8
    // - only used by `epoll_create`
    static constexpr int _SIZE_ARGUMENT_FOR_EPOLL_CREATE = 1;

    // - only used by `epoll_create1` (added to the kernel in version 2.6.27)
    static constexpr int _EPOLL_CREATE1_FLAGS = EPOLL_CLOEXEC;

    int _epoll_fd;
    epoll_event _event_array[_MAX_SIZE_OF_EVENT_ARRAY];
    std::unordered_set<int> _fds_that_are_listening_on;
};

} // namespace xubinh_server

#endif