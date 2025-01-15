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
#include "util/this_thread.h"
#ifdef __USE_LOCK_FREE_QUEUE
#include "util/lock_free_queue.h"
#else
#include "util/blocking_queue.h"
#endif

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
#ifdef __USE_LOCK_FREE_QUEUE
    using FunctorQueue = util::SpscLockFreeQueue<EventLoop::FunctorType>;
#else
    using FunctorQueue = util::BlockingQueue<EventLoop::FunctorType>;
#endif

    static void set_alarm_advancing_threshold(int64_t alarm_advancing_threshold
    ) noexcept {
        _alarm_advancing_threshold = alarm_advancing_threshold;
    }

    // should be initialized in the owner thread in order to get the tid
    // properly
    EventLoop(
        uint64_t loop_index = 0, size_t number_of_functor_blocking_queues = 1
    );

    ~EventLoop() noexcept;

    uint64_t get_loop_index() const noexcept {
        return _loop_index;
    }

    bool is_in_owner_thread() {
        return util::this_thread::get_tid() == _owner_thread_tid;
    }

    void loop();

    // not thread-safe
    //
    // - this method is tied up with `EventDispatcher` which in turn is tied up
    // with this exact loop, therefore will always be executed sequentially
    void register_event_for_fd(int fd, const epoll_event *event);

    void detach_fd_from_poller(int fd);

    void run(FunctorType functor, uint64_t functor_blocking_queue_index = 0);

    TimerIdentifier run_at_time_point(
        TimePoint time_point,
        TimeInterval repetition_time_interval,
        int number_of_repetitions,
        FunctorType functor,
        uint64_t functor_blocking_queue_index = 0
    );

    TimerIdentifier run_after_time_interval(
        TimeInterval time_interval,
        TimeInterval repetition_time_interval,
        int number_of_repetitions,
        FunctorType functor,
        uint64_t functor_blocking_queue_index = 0
    );

    void cancel_a_timer(TimerIdentifier timer_identifier);

    // not thread-safe
    void ask_to_stop();

private:
    void _leave_to_owner_thread(
        FunctorType functor, uint64_t functor_blocking_queue_index
    );

    void _wake_up_this_loop(uint64_t functor_blocking_queue_index);

    void _invoke_all_functors();

    void _set_alarm_at_time_point(TimePoint time_point) {
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
    void _expire_and_update_alarm(TimePoint time_point);

    // for releasing resources
    void _release_all_timers();

    // for async-handling of eventfd notifications
    void _eventfd_message_callback(uint64_t value);

    // for async-handling of timerfd notifications
    void _timerfd_message_callback(uint64_t value);

#ifndef __USE_LOCK_FREE_QUEUE
    static constexpr int _FUNCTOR_QUEUE_CAPACITY = 1000;
#endif

    // in seconds
    static int64_t _alarm_advancing_threshold;

    const uint64_t _loop_index;

    EventPoller _event_poller;

    const size_t _number_of_functor_blocking_queues;
    std::vector<FunctorQueue *> _functor_blocking_queues;
    std::vector<Eventfd *> _eventfds;
    std::vector<std::atomic<bool>> _eventfd_pilot_lamps;
    bool _eventfd_triggered = false;

    Timerfd _timerfd;
    TimerContainer _timer_container{};
    bool _timerfd_triggered = false;
    TimePoint _next_earliest_expiration_time{TimePoint::FOREVER};

    pid_t _owner_thread_tid;

    std::atomic<bool> _need_stop{false};
};

} // namespace xubinh_server

#endif