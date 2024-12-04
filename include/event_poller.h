#ifndef XUBINH_SERVER_EVENT_POLLER
#define XUBINH_SERVER_EVENT_POLLER

#include <sys/epoll.h>
#include <unordered_set>
#include <vector>

#include "event_dispatcher.h"
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

    std::vector<EventDispatcher *> poll_for_active_events_of_all_fds();

private:
    static constexpr int _MAX_SIZE_OF_EVENT_ARRAY = 65536;

    int _epoll_fd;
    epoll_event _event_array[_MAX_SIZE_OF_EVENT_ARRAY]{};
    std::unordered_set<int> _fds_that_are_listening_on;
};

} // namespace xubinh_server

#endif