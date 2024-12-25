#include <sys/epoll.h>

#include "event_poller.h"
#include "log_builder.h"

namespace xubinh_server {

EventPoller::EventPoller() : _epoll_fd(epoll_create1(_EPOLL_CREATE1_FLAGS)) {

    if (_epoll_fd == -1) {
        LOG_SYS_FATAL << "epoll_create1 failed";
    }
}

EventPoller::~EventPoller() {
    if (!_fds_that_are_listening_on.empty()) {
        LOG_WARN << "poller closed while there are still fds being listened on";
    }

    ::close(_epoll_fd);
}

void EventPoller::register_event_for_fd(int fd, const epoll_event *event) {
    auto it = _fds_that_are_listening_on.find(fd);

    bool is_attached = it != _fds_that_are_listening_on.end();

    decltype(EPOLL_CTL_ADD) epoll_ctl_method_type =
        is_attached ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;

    if (::epoll_ctl(
            _epoll_fd,
            epoll_ctl_method_type,
            fd,
            const_cast<epoll_event *>(event)
        )
        == -1) {

        LOG_SYS_FATAL << "epoll_ctl failed";
    }

    if (!is_attached) {
        _fds_that_are_listening_on.insert(fd);
    }
}

void EventPoller::detach_fd(int fd) {
    auto it = _fds_that_are_listening_on.find(fd);

    bool is_attached = it != _fds_that_are_listening_on.end();

    if (!is_attached) {
        LOG_WARN << "try to detach a fd that is not yet attached";
    }

    if (::epoll_ctl(
            _epoll_fd,
            EPOLL_CTL_DEL,
            fd,
            nullptr // buggy in kernel versions before 2.6.9
        )
        == -1) {

        LOG_SYS_FATAL << "epoll_ctl failed";
    }

    _fds_that_are_listening_on.erase(fd);
}

void EventPoller::poll_for_active_events_of_all_fds(
    std::vector<PollableFileDescriptor *> &event_dispatchers
) {
    LOG_TRACE << "epoll_wait blocked";

    int current_event_array_size =
        ::epoll_wait(_epoll_fd, _event_array, _MAX_SIZE_OF_EVENT_ARRAY, -1);

    LOG_TRACE << "epoll_wait resume";

    if (current_event_array_size == -1) {
        LOG_SYS_FATAL << "epoll_wait failed";
    }

    event_dispatchers.clear();

    for (int i = 0; i < current_event_array_size; i++) {
        auto event_dispatcher_ptr =
            static_cast<PollableFileDescriptor *>(_event_array[i].data.ptr);

        event_dispatcher_ptr->set_active_events(_event_array[i].events);

        event_dispatchers.push_back(event_dispatcher_ptr);
    }

    return;
}

} // namespace xubinh_server