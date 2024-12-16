#ifndef __XUBINH_SERVER_SIGNALFD
#define __XUBINH_SERVER_SIGNALFD

#include <csignal>
#include <functional>
#include <sys/signalfd.h>

#include "event_loop.h"
#include "pollable_file_descriptor.h"

namespace xubinh_server {

class SignalSet {
public:
    SignalSet() {
        ::sigemptyset(&_signal_set);
    }

    void add_signal(int signal);

    void remove_signal(int signal);

private:
    friend class Signalfd;

    sigset_t _signal_set{};
};

class Signalfd {
public:
    using SignalDispatcherType = std::function<void(int signal)>;

    // only blocks calling thread's signals
    static void block_signals(SignalSet signals_to_block);

    // only unblocks calling thread's signals
    static void unblock_signals(SignalSet signals_to_unblock);

    static int create_signalfd(SignalSet initial_listening_set, int flags);

    Signalfd(int fd, EventLoop *loop) : _pollable_file_descriptor(fd, loop) {
        if (fd < 0) {
            LOG_SYS_FATAL << "invalid file descriptor (must be non-negative)";
        }

        _pollable_file_descriptor.register_read_event_callback(
            std::bind(&Signalfd::_read_event_callback, this)
        );
    }

    ~Signalfd() {
        _pollable_file_descriptor.detach_from_poller();

        ::close(_pollable_file_descriptor.get_fd());
    }

    void register_signal_dispatcher(SignalDispatcherType signal_dispatcher) {
        _signal_dispatcher = std::move(signal_dispatcher);
    }

    void start() {
        if (!_signal_dispatcher) {
            LOG_FATAL << "missing signal dispatcher";
        }

        _pollable_file_descriptor.enable_read_event();
    }

private:
    static constexpr int _DEFAULT_SIGNALFD_FLAGS = SFD_NONBLOCK | SFD_CLOEXEC;

    void _read_event_callback();

    SignalDispatcherType _signal_dispatcher;

    PollableFileDescriptor _pollable_file_descriptor;
};

} // namespace xubinh_server

#endif