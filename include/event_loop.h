#ifndef __XUBINH_SERVER_EVENT_LOOP
#define __XUBINH_SERVER_EVENT_LOOP

#include <atomic>
#include <functional>
#include <unistd.h>

#include "event_poller.h"
#include "eventfd.h"
#include "timer.h"
#include "timer_container.h"
#include "timer_identifier.h"
#include "timerfd.h"
#include "util/blocking_queue.h"
#include "util/current_thread.h"

namespace xubinh_server {

// Abstraction of an event loop
//
// - Note that user of this class should initialize the object in the owner
// thread and return a pointer back to the caller thread
class EventLoop {
private:
    using TimePoint = util::TimePoint;
    using TimeInterval = util::TimeInterval;

public:
    using FunctorType = std::function<void()>;
    using FunctorQueue = util::BlockingQueue<EventLoop::FunctorType>;

    // should be initialized in the owner thread in order to get the tid
    // properly
    EventLoop();

    ~EventLoop() {
        _release_all_timers();
    };

    void loop();

    // not thread-safe
    //
    // - this method is tied up with `EventDispatcher` which in turn is tied up
    // with this exact loop, therefore will always be executed sequentially
    void register_event_for_fd(int fd, const epoll_event *event) {
        LOG_TRACE << "enter event: register_event_for_fd";

        _event_poller.register_event_for_fd(fd, event);
    }

    void detach_fd_from_poller(int fd) {
        LOG_TRACE << "enter event: detach_fd_from_poller";

        _event_poller.detach_fd(fd);
    }

    void run(FunctorType functor);

    TimerIdentifier run_at_time_point(
        const TimePoint &time_point,
        const TimeInterval &repetition_time_interval,
        int number_of_repetitions,
        FunctorType functor
    );

    TimerIdentifier run_after_time_interval(
        const TimeInterval &time_interval,
        const TimeInterval &repetition_time_interval,
        int number_of_repetitions,
        FunctorType functor
    );

    void cancel_a_timer(const TimerIdentifier &timer_identifier);

    // not thread-safe
    void ask_to_stop();

private:
    static constexpr int _FUNCTOR_QUEUE_CAPACITY = 1000;

    bool _is_in_owner_thread() {
        return util::current_thread::get_tid() == _owner_thread_tid;
    }

    void _leave_to_owner_thread(FunctorType functor) {
        _functor_blocking_queue.push(std::move(functor));

        _wake_up_this_loop();
    }

    void _wake_up_this_loop() {
        _eventfd.increment_by_value(1);
    }

    void _set_alarm_at_time_point(const TimePoint &time_point) {
        _timerfd.set_alarm_at_time_point(time_point);
    }

    void _cancel_alarm() {
        _timerfd.cancel();
    }

    // may be called by both the user and the loop itself
    void _add_a_timer_and_update_alarm(const Timer *timer_ptr);

    // may be called by both the user and the loop itself
    void _cancel_a_timer_and_update_alarm(const Timer *timer_ptr);

    // expire all timers before or at given time point and update alarm
    //
    // - only be called by the loop itself
    void _expire_and_update_alarm(const TimePoint &time_point);

    // for releasing resources
    void _release_all_timers();

    // for async-handling of eventfd notifications
    void _eventfd_message_callback(uint64_t value);

    // for async-handling of timerfd notifications
    void _timerfd_message_callback(uint64_t value);

    EventPoller _event_poller;

    Eventfd _eventfd;
    bool _eventfd_triggered = false;
    FunctorQueue _functor_blocking_queue;

    Timerfd _timerfd;
    bool _timerfd_triggered = false;
    TimerContainer _timer_container;

    pid_t _owner_thread_tid;

    std::atomic<bool> _need_stop{false};
};

} // namespace xubinh_server

#endif