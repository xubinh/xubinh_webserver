#include <sys/epoll.h>
#include <sys/resource.h>

#include "event_poller.h"
#include "log_builder.h"

namespace xubinh_server {

size_t EventPoller::get_max_number_limit_of_file_descriptors_per_process() {
    struct rlimit limit;

    if (::getrlimit(RLIMIT_NOFILE, &limit) == -1) {
        LOG_SYS_FATAL << "getrlimit failed";
    }

    return static_cast<size_t>(limit.rlim_cur);
}

EventPoller::EventPoller() : _epoll_fd(epoll_create1(_EPOLL_CREATE1_FLAGS)) {
    if (_epoll_fd == -1) {
        LOG_SYS_FATAL << "epoll_create1 failed";
    }
}

EventPoller::~EventPoller() {
    if (_number_of_fds_that_are_listening_on) {
        LOG_WARN << "poller closed while there are still fds being listened on";
    }

    ::close(_epoll_fd);

    delete[] _event_array;
    delete[] _fds_that_are_listening_on;
}

void EventPoller::register_event_for_fd(int fd, const epoll_event *event) {
    bool is_attached = _fds_that_are_listening_on[fd];

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
        _fds_that_are_listening_on[fd] = true;
        _number_of_fds_that_are_listening_on += 1;
    }
}

void EventPoller::detach_fd(int fd) {
    bool is_attached = _fds_that_are_listening_on[fd];

    if (!is_attached) {
        LOG_WARN << "try to detach a fd that is not yet attached";

        return;
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

    _fds_that_are_listening_on[fd] = false;
    _number_of_fds_that_are_listening_on -= 1;
}

void EventPoller::poll_for_active_events_of_all_fds(
    std::vector<PollableFileDescriptor *> &event_dispatchers
) {
    LOG_TRACE << "epoll_wait blocked";

    int current_event_array_size;

    // epoll_wait might be interrupted by signal handlers, so make a loop for it
    while (true) {
        current_event_array_size =
            ::epoll_wait(_epoll_fd, _event_array, _MAX_SIZE_OF_EVENT_ARRAY, -1);

        if (current_event_array_size == -1) {
            if (errno == EINTR) {
                LOG_TRACE << "epoll_wait got interrupted by signal handlers";

                continue;
            }

            else {
                LOG_SYS_FATAL << "epoll_wait failed";
            }
        }

        else {
            break;
        }
    }

    LOG_TRACE << "epoll_wait resume";

    event_dispatchers.clear();

    for (int i = 0; i < current_event_array_size; i++) {
        auto event_dispatcher_ptr =
            static_cast<PollableFileDescriptor *>(_event_array[i].data.ptr);

        event_dispatcher_ptr->set_active_events(_event_array[i].events);

        event_dispatchers.push_back(event_dispatcher_ptr);
    }

    return;
}

const size_t EventPoller::_MAX_SIZE_OF_EVENT_ARRAY =
    get_max_number_limit_of_file_descriptors_per_process();

} // namespace xubinh_server