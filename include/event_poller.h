#ifndef __XUBINH_SERVER_EVENT_POLLER
#define __XUBINH_SERVER_EVENT_POLLER

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
            EPOLLIN | EPOLLPRI
            | EPOLLRDHUP, // EPOLLRDHUP here stands for the fact
                          // that EOF itself is also a kind of readable data
                          // that should be read by the callback in order to
                          // update the internal state of the connection
        EVENT_TYPE_WRITE = EPOLLOUT,
        EVENT_TYPE_CLOSE = EPOLLHUP,
        EVENT_TYPE_ERROR = EPOLLERR,
        EVENT_TYPE_ALL = EVENT_TYPE_READ | EVENT_TYPE_WRITE
    };

    static size_t
    get_limit_of_max_number_of_opened_file_descriptors_per_process();

    static void set_limit_of_max_number_of_opened_file_descriptors_per_process(
        int max_limit_of_open_fd
    );

    EventPoller();

    ~EventPoller();

    void register_event_for_fd(int fd, const epoll_event *event);

    void detach_fd(int fd);

    size_t size() const {
        return _number_of_fds_that_are_listening_on;
    }

    bool empty() const {
        return size() == 0;
    }

    // pass by reference to prevent unnecessary memory allocations
    void poll_for_active_events_of_all_fds(
        std::vector<PollableFileDescriptor *> &event_dispatchers
    );

private:
    // - ignored but still must be greater than zero since Linux 2.6.8
    // - only used by `epoll_create`
    static constexpr int _SIZE_ARGUMENT_FOR_EPOLL_CREATE = 1;

    // - only used by `epoll_create1` (added to the kernel in version 2.6.27)
    static constexpr int _EPOLL_CREATE1_FLAGS = EPOLL_CLOEXEC;

    static const size_t _MAX_SIZE_OF_EVENT_ARRAY;

    int _epoll_fd;
    epoll_event *_event_array{new epoll_event[_MAX_SIZE_OF_EVENT_ARRAY]};
    bool *_fds_that_are_listening_on{new bool[_MAX_SIZE_OF_EVENT_ARRAY]{false}};
    size_t _number_of_fds_that_are_listening_on{0};
};

} // namespace xubinh_server

#endif