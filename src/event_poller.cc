#include <sys/epoll.h>
#include <sys/resource.h>

#include "event_poller.h"
#include "log_builder.h"

namespace xubinh_server {

size_t
EventPoller::get_limit_of_max_number_of_opened_file_descriptors_per_process() {
    struct rlimit limit;

    if (::getrlimit(RLIMIT_NOFILE, &limit) == -1) {
        LOG_SYS_FATAL << "getrlimit failed";
    }

    return static_cast<size_t>(limit.rlim_cur);
}

void EventPoller::
    set_limit_of_max_number_of_opened_file_descriptors_per_process(
        size_t max_limit_of_open_fd
    ) {

    LOG_INFO
        << "setting limit of max number of opened file descriptors per process";

    struct rlimit limit;

    if (::getrlimit(RLIMIT_NOFILE, &limit) == -1) {
        LOG_SYS_FATAL << "getrlimit failed";
    }

    LOG_INFO << "current limit: " << limit.rlim_cur << " (rlim_cur), "
             << limit.rlim_max << " (rlim_max)";

    // [NOTE]: consider the value carefully; a value no greater than the hard
    // limit might still crash the system if being close to it
    limit.rlim_cur =
        std::min(static_cast<rlim_t>(max_limit_of_open_fd), limit.rlim_max);

    LOG_INFO << "new limit: " << limit.rlim_cur << " (rlim_cur), "
             << limit.rlim_max << " (rlim_max)";

    // [NOTE]: this will set both the soft limit and the hard limit to the
    // specified value, according to the man page
    if (::setrlimit(RLIMIT_NOFILE, &limit) == -1) {
        LOG_SYS_FATAL << "setrlimit failed";
    }

    if (::getrlimit(RLIMIT_NOFILE, &limit) == -1) {
        LOG_SYS_FATAL << "getrlimit failed";
    }

    LOG_INFO << "setrlimit succeeded, current limit: " << limit.rlim_cur
             << " (rlim_cur), " << limit.rlim_max << " (rlim_max)";
}

EventPoller::EventPoller()
    : _epoll_fd(epoll_create1(_EPOLL_CREATE1_FLAGS)) {

    if (_epoll_fd == -1) {
        LOG_SYS_FATAL << "epoll_create1 failed";
    }

    _max_size_of_event_array =
        get_limit_of_max_number_of_opened_file_descriptors_per_process();

    _event_array = new epoll_event[_max_size_of_event_array];
    _fds_that_are_listening_on = new bool[_max_size_of_event_array]{false};
}

EventPoller::~EventPoller() {
    if (_number_of_fds_that_are_listening_on) {
        LOG_WARN << "poller closed while there are still fds being listened on";
    }

    ::close(_epoll_fd);

    delete[] _fds_that_are_listening_on;
    delete[] _event_array;
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

        LOG_SYS_FATAL << "epoll_ctl failed, fd: " << fd;
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

        LOG_SYS_FATAL << "epoll_ctl failed, fd: " << fd;
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
        current_event_array_size = ::epoll_wait(
            _epoll_fd,
            _event_array,
            static_cast<int>(_max_size_of_event_array),
            -1
        );

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

} // namespace xubinh_server
