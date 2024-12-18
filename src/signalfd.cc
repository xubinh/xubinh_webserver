#include "signalfd.h"
#include "log_builder.h"

namespace xubinh_server {

void SignalSet::add_signal(int signal) {
    if (signal == SIGKILL || signal == SIGSTOP) {
        LOG_FATAL << "tried to add SIGKILL and SIGSTOP to a signal set";
    }

    ::sigaddset(&_signal_set, signal);
}

void SignalSet::remove_signal(int signal) {
    if (signal == SIGKILL || signal == SIGSTOP) {
        LOG_WARN << "tried to remove SIGKILL and SIGSTOP from a signal set";
    }

    ::sigdelset(&_signal_set, signal);
}

void Signalfd::block_signals(SignalSet signals_to_block) {
    if (::sigprocmask(SIG_BLOCK, &signals_to_block._signal_set, nullptr)) {
        LOG_SYS_FATAL << "unknown error when blocking signals";
    }
}

void Signalfd::unblock_signals(SignalSet signals_to_unblock) {
    if (::sigprocmask(SIG_UNBLOCK, &signals_to_unblock._signal_set, nullptr)) {
        LOG_SYS_FATAL << "unknown error when unblocking signals";
    }
}

int Signalfd::create_signalfd(SignalSet initial_listening_set, int flags) {
    if (!flags) {
        flags = _DEFAULT_SIGNALFD_FLAGS;
    }

    auto fd = ::signalfd(-1, &initial_listening_set._signal_set, flags);

    if (fd == -1) {
        if (errno == EINVAL) {
            LOG_FATAL << "invalid flags are given to create signalfd";
        }
        else {
            LOG_SYS_FATAL << "failed when trying to create signalfd";
        }
    }

    return fd;
}

void Signalfd::_read_event_callback(util::TimePoint time_stamp) {
    struct signalfd_siginfo signal_info;

    // one signal at a time
    while (true) {
        auto bytes_read = ::read(
            _pollable_file_descriptor.get_fd(),
            &signal_info,
            sizeof(signal_info)
        );

        if (bytes_read != sizeof(signal_info)) {
            // all signals in the pending queue are read
            if (errno == EAGAIN) {
                break;
            }

            else {
                LOG_SYS_FATAL << "unknown error occured when trying to read "
                                 "from signalfd";
            }
        }

        _signal_dispatcher(static_cast<int>(signal_info.ssi_signo));
    }
}

} // namespace xubinh_server