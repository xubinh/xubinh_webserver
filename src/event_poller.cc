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
        LOG_ERROR
            << "poller closed while there are still fds being listened on";
    }

    ::close(_epoll_fd);
}

void EventPoller::register_event_for_fd(int fd, const epoll_event *event) {
    auto found_iterator = _fds_that_are_listening_on.find(fd);

    bool is_already_listening_on =
        found_iterator != _fds_that_are_listening_on.end();

    bool need_to_delete = !(event->events & EventPoller::EVENT_TYPE_ALL);

    decltype(EPOLL_CTL_ADD) which_epoll_ctl_function = EPOLL_CTL_ADD;

    if (is_already_listening_on) {
        if (need_to_delete) {
            which_epoll_ctl_function = EPOLL_CTL_DEL;

            _fds_that_are_listening_on.erase(found_iterator);
        }

        else {
            which_epoll_ctl_function = EPOLL_CTL_MOD;
        }
    }

    else {
        if (need_to_delete) {
            LOG_WARN << "try to EPOLL_CTL_DEL a fd that hasn't meet before";

            return;
        }

        else {
            which_epoll_ctl_function = EPOLL_CTL_ADD;

            _fds_that_are_listening_on.insert(fd);
        }
    }

    if (epoll_ctl(
            _epoll_fd,
            which_epoll_ctl_function,
            fd,
            const_cast<epoll_event *>(event)
        )
        == -1) {

        LOG_SYS_FATAL << "epoll_ctl failed";
    }
}

std::vector<PollableFileDescriptor *>
EventPoller::poll_for_active_events_of_all_fds() {
    int current_event_array_size =
        epoll_wait(_epoll_fd, _event_array, _MAX_SIZE_OF_EVENT_ARRAY, -1);

    if (current_event_array_size == -1) {
        LOG_SYS_FATAL << "epoll_wait failed";
    }

    std::vector<PollableFileDescriptor *> event_dispatchers;

    for (int i = 0; i < current_event_array_size; i++) {
        auto event_dispatcher_ptr =
            static_cast<PollableFileDescriptor *>(_event_array[i].data.ptr);

        event_dispatcher_ptr->set_active_events(_event_array[i].events);

        event_dispatchers.push_back(event_dispatcher_ptr);
    }

    return event_dispatchers;
}

} // namespace xubinh_server