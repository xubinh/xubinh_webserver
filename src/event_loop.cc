#include "event_loop.h"
#include "log_builder.h"
#include "util/current_thread.h"

namespace xubinh_server {

EventLoop::EventLoop(
    uint64_t loop_index, size_t number_of_functor_blocking_queues
)
    : _loop_index(loop_index),
      _number_of_functor_blocking_queues(
          std::max(number_of_functor_blocking_queues, static_cast<size_t>(1))
      ),
      _functor_blocking_queues(_number_of_functor_blocking_queues),
      _eventfds(_number_of_functor_blocking_queues),
      _eventfd_pilot_lamps(_number_of_functor_blocking_queues),
      _timerfd(Timerfd::create_timerfd(0), this),
      _owner_thread_tid(util::current_thread::get_tid()) {

    for (int i = 0; i < _number_of_functor_blocking_queues; i++) {
#ifdef __USE_LOCK_FREE_QUEUE
        _functor_blocking_queues[i] = new FunctorQueue;
#else
        _functor_blocking_queues[i] = new FunctorQueue(_FUNCTOR_QUEUE_CAPACITY);
#endif

        _eventfds[i] = new Eventfd(Eventfd::create_eventfd(0), this);
        auto &eventfd_pilot_lamp = _eventfd_pilot_lamps[i];
        _eventfds[i]->register_eventfd_message_callback(
            [this, &eventfd_pilot_lamp](uint64_t value) {
                eventfd_pilot_lamp.store(false, std::memory_order_relaxed);

                _eventfd_message_callback(value);
            }
        );

        _eventfds[i]->start();

        _eventfd_pilot_lamps[i].store(false, std::memory_order_relaxed);
    }

    _timerfd.register_timerfd_message_callback([this](uint64_t value) {
        _timerfd_message_callback(value);
    });

    _timerfd.start();

    // for debug purpose; yell to prove the thread is not dead
    int64_t yell_interval = 3 * TimeInterval::SECOND;
    run_after_time_interval(yell_interval, yell_interval, -1, []() {
        LOG_DEBUG << "I'm not dead";
    });
}

EventLoop::~EventLoop() noexcept {
    _release_all_timers();

    for (auto ptr : _functor_blocking_queues) {
        delete ptr;
    }

    for (auto ptr : _eventfds) {
        delete ptr;
    }
};

void EventLoop::loop() {
    std::vector<PollableFileDescriptor *> event_dispatchers;

    while (true) {
        // checks for stop signal (either external or from within) and stops
        // only when the polling list is logically empty (which really is not
        // since each event loop always got two fixed fd's being listened on,
        // i.e. a timerfd and an eventfd)
        if (_need_stop.load(std::memory_order_relaxed)) {

            if (_event_poller.size()
                == 1 + _number_of_functor_blocking_queues) {
                LOG_INFO << "event loop exits, target TID: "
                         << _owner_thread_tid;

                break;
            }

            else {
                LOG_INFO << "non-empty event poller, size: "
                         << _event_poller.size()
                         << ", event loop can not exit now, target TID: "
                         << _owner_thread_tid;
            }
        }

        _event_poller.poll_for_active_events_of_all_fds(event_dispatchers);

        LOG_TRACE << "number of dispatchers: "
                         + std::to_string(event_dispatchers.size());

        util::TimePoint time_stamp;

        // if the event dispatcher pointer is polled out, it is ensured to be a
        // valid pointer since the unregistering of event dispatchers is fully
        // delegated to the loop itself and is therefore thread-safe
        for (auto event_dispatcher_ptr : event_dispatchers) {
            // the lifetime guard inside the event dispatcher is for preventing
            // self-destruction done by itself
            event_dispatcher_ptr->dispatch_active_events(time_stamp);
        }

        LOG_TRACE
            << "dispatching completed, next for checking eventfd and timerfd";

        LOG_TRACE << "checking eventfd...";

        if (_eventfd_triggered) {
            LOG_TRACE << "eventfd triggered";

            _invoke_all_functors();

            _eventfd_triggered = false;
        }

        else {
            LOG_TRACE << "nothing happened on eventfd";
        }

        LOG_TRACE << "checking timerfd...";

        if (_timerfd_triggered) {
            LOG_TRACE << "timerfd triggered";

            TimePoint current_time_point;

            _expire_and_update_alarm(current_time_point);

            _timerfd_triggered = false;
        }

        else {
            LOG_TRACE << "nothing happened on timerfd";
        }

        LOG_TRACE << "current size of poller: " << _event_poller.size();
    }

    // clean up the functors before exiting
    _invoke_all_functors();
}

void EventLoop::register_event_for_fd(int fd, const epoll_event *event) {
    LOG_TRACE << "enter event: register_event_for_fd";

    _event_poller.register_event_for_fd(fd, event);
}

void EventLoop::detach_fd_from_poller(int fd) {
    LOG_TRACE << "enter event: detach_fd_from_poller";

    _event_poller.detach_fd(fd);
}

void EventLoop::run(
    FunctorType functor, uint64_t functor_blocking_queue_index
) {
    if (is_in_owner_thread()) {
        functor();
    }

    else {
        _leave_to_owner_thread(
            std::move(functor), functor_blocking_queue_index
        );
    }
}

// - a `0` of repetition time interval means one-off timer
// - a `-1` of repetition number means infinite repetition
TimerIdentifier EventLoop::run_at_time_point(
    TimePoint time_point,
    TimeInterval repetition_time_interval,
    int number_of_repetitions,
    FunctorType functor,
    uint64_t functor_blocking_queue_index
) {
    Timer *timer_ptr = new Timer(
        time_point,
        repetition_time_interval,
        number_of_repetitions,
        std::move(functor)
    );

    run(
        [this, timer_ptr]() {
            _add_a_timer_and_update_alarm(timer_ptr);
        },
        functor_blocking_queue_index
    );

    return TimerIdentifier{timer_ptr};
}

// - a `0` of repetition time interval means one-off timer
// - a `-1` of repetition number means infinite repetition
TimerIdentifier EventLoop::run_after_time_interval(
    TimeInterval time_interval,
    TimeInterval repetition_time_interval,
    int number_of_repetitions,
    FunctorType functor,
    uint64_t functor_blocking_queue_index
) {
    TimePoint time_point = (TimePoint() += time_interval);

    TimerIdentifier timer_identifier = run_at_time_point(
        time_point,
        repetition_time_interval,
        number_of_repetitions,
        std::move(functor),
        functor_blocking_queue_index
    );

    return timer_identifier;
}

void EventLoop::cancel_a_timer(TimerIdentifier timer_identifier) {
    const Timer *timer_ptr = timer_identifier._timer_ptr;

    run([this, timer_ptr]() {
        _cancel_a_timer_and_update_alarm(timer_ptr);
    });
}

void EventLoop::ask_to_stop() {
    LOG_INFO << "asking loop to stop, target TID: " << _owner_thread_tid;

    // a release fence that comes after a store essentially makes the store
    // become a part of the memory ordering, i.e. store first and then do the
    // wake-up
    _need_stop.store(true, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_release);

    _wake_up_this_loop(0);

    LOG_INFO << "wake-up signal are sent to the loop";
}

void EventLoop::_leave_to_owner_thread(
    FunctorType functor, uint64_t functor_blocking_queue_index
) {
    _functor_blocking_queues[functor_blocking_queue_index]->push(
        std::move(functor)
    );

    LOG_TRACE << "leave to owner thread (TID: " << _owner_thread_tid
              << "), functor queue index: " << (functor_blocking_queue_index)
              << ", current thread TID: " << util::current_thread::get_tid();

    _wake_up_this_loop(functor_blocking_queue_index);
}

void EventLoop::_wake_up_this_loop(uint64_t functor_blocking_queue_index) {
    bool expected = false;

    // send to eventfd only when the pilot lamp is off
    if (_eventfd_pilot_lamps[functor_blocking_queue_index]
            .compare_exchange_strong(
                expected, true, std::memory_order_relaxed
            )) {

        _eventfds[functor_blocking_queue_index]->increment_by_value(1);
    }
}

void EventLoop::_invoke_all_functors() {
    for (auto &_functor_blocking_queue_ptr : _functor_blocking_queues) {
#ifdef __USE_LOCK_FREE_QUEUE
#ifdef __USE_LOCK_FREE_QUEUE_WITH_RAW_POINTER
        FunctorType *functor_ptr;

        while (functor_ptr = _functor_blocking_queue_ptr->pop()) {
            (*functor_ptr)();
            delete functor_ptr;
        }
#else
        std::shared_ptr<FunctorType> functor_ptr;

        while (functor_ptr = _functor_blocking_queue_ptr->pop()) {
            (*functor_ptr)();
        }
#endif
#else
#ifdef __USE_BLOCKING_QUEUE_WITH_RAW_POINTER
        const auto &queued_functors = _functor_blocking_queue_ptr->pop_all();

        for (auto &functor : queued_functors) {
            (*functor)();

            delete functor;
        }
#else
        const auto &queued_functors = _functor_blocking_queue_ptr->pop_all();

        for (auto &functor : queued_functors) {
            functor();
        }
#endif
#endif
    }
}

void EventLoop::_add_a_timer_and_update_alarm(const Timer *timer_ptr) {
    LOG_TRACE << "entering `_add_a_timer_and_update_alarm`";

    _timer_container.insert_one(timer_ptr);

    TimePoint earliest_expiration_time_point_after_insertion =
        _timer_container.get_earliest_expiration_time_point();

    LOG_TRACE << "earliest_expiration_time_point_after_insertion: "
              << earliest_expiration_time_point_after_insertion
                     .to_datetime_string(TimePoint::Purpose::PRINTING);

    // newly inserted timer advanced the expiration
    if (earliest_expiration_time_point_after_insertion
        < _next_earliest_expiration_time
              - util::TimeInterval(_alarm_advancing_threshold)) {

        _set_alarm_at_time_point(earliest_expiration_time_point_after_insertion
        );

        _next_earliest_expiration_time =
            earliest_expiration_time_point_after_insertion;

        LOG_TRACE << "alarm advanced at: "
                  << _next_earliest_expiration_time.to_datetime_string(
                         TimePoint::Purpose::PRINTING
                     );
    }

    else {
        LOG_TRACE << "alarm remain the same";
    }

    LOG_TRACE << "exiting `_add_a_timer_and_update_alarm`";
}

void EventLoop::_cancel_a_timer_and_update_alarm(const Timer *timer_ptr) {
    // might be expired already
    bool flag_timer_not_exist = !_timer_container.remove_one(timer_ptr);

    if (flag_timer_not_exist) {
        return;
    }

    // cancel alarm if there were no timers left
    if (_timer_container.empty()) {
        _cancel_alarm();

        _next_earliest_expiration_time = TimePoint::FOREVER;

        return;
    }

    TimePoint earliest_expiration_time_point_after_removal =
        _timer_container.get_earliest_expiration_time_point();

    // or reset alarm if expiration time is delayed by the removal
    if (_next_earliest_expiration_time
        < earliest_expiration_time_point_after_removal) {

        _set_alarm_at_time_point(earliest_expiration_time_point_after_removal);

        _next_earliest_expiration_time =
            earliest_expiration_time_point_after_removal;
    }

    delete timer_ptr;
}

void EventLoop::_expire_and_update_alarm(TimePoint time_point) {
    LOG_TRACE << "entering _expire_and_update_alarm";

    std::vector<const Timer *> expired_timers =
        _timer_container.move_out_before_or_at(time_point);

    LOG_TRACE << "moved out " << expired_timers.size() << " timers";

    LOG_TRACE << "number of timers left in the container: "
              << _timer_container.size();

    // update alarm status for the callbacks
    if (_timer_container.empty()) {
        _cancel_alarm();

        _next_earliest_expiration_time = TimePoint::FOREVER;

        LOG_TRACE << "no timers left currently; cancelling alarm";
    }
    else {
        TimePoint earliest_expiration_time_point_after_removal =
            _timer_container.get_earliest_expiration_time_point();

        if (_next_earliest_expiration_time
            < earliest_expiration_time_point_after_removal) {

            _set_alarm_at_time_point(
                earliest_expiration_time_point_after_removal
            );

            _next_earliest_expiration_time =
                earliest_expiration_time_point_after_removal;

            LOG_TRACE << "alarm delayed at: "
                      << earliest_expiration_time_point_after_removal
                             .to_datetime_string(TimePoint::Purpose::PRINTING);
        }

        else {
            LOG_TRACE << "alarm remain the same";
        }
    }

    std::vector<const Timer *> timers_that_are_still_valid;

    LOG_TRACE << "expiring timers...";

    // the callbacks themselves might also add new timers and update the alarm
    for (const Timer *timer_ptr : expired_timers) {
        auto temp_mutable_timer_ptr = const_cast<Timer *>(timer_ptr);

        if (temp_mutable_timer_ptr->expire_until(time_point)) {
            timers_that_are_still_valid.push_back(timer_ptr);
        }

        else {
            delete timer_ptr;
        }
    }

    LOG_TRACE << "finished expiring timers";

    LOG_TRACE << "number of timers left in the container: "
              << _timer_container.size();

    if (timers_that_are_still_valid.empty()) {
        LOG_TRACE << "no valid timers left";
        LOG_TRACE << "exiting _expire_and_update_alarm";

        return;
    }

    LOG_TRACE << "number of valid timers: "
              << timers_that_are_still_valid.size();

    LOG_TRACE << "inserting valid timers...";

    _timer_container.insert_all(timers_that_are_still_valid);

    LOG_TRACE << "number of timers left in the container: "
              << _timer_container.size();

    TimePoint earliest_expiration_time_point_after_inserting_all =
        _timer_container.get_earliest_expiration_time_point();

    // newly inserted timer advanced the expiration
    if (earliest_expiration_time_point_after_inserting_all
        < _next_earliest_expiration_time
              - util::TimeInterval(_alarm_advancing_threshold)) {

        _set_alarm_at_time_point(
            earliest_expiration_time_point_after_inserting_all
        );

        _next_earliest_expiration_time =
            earliest_expiration_time_point_after_inserting_all;

        LOG_TRACE << "alarm advanced at: "
                  << _next_earliest_expiration_time.to_datetime_string(
                         TimePoint::Purpose::PRINTING
                     );
    }

    LOG_TRACE << "exiting _expire_and_update_alarm";
}

void EventLoop::_release_all_timers() {
    std::vector<const Timer *> all_timers =
        _timer_container.move_out_before_or_at(TimePoint::FOREVER);

    for (const Timer *timer_ptr : all_timers) {
        delete timer_ptr;
    }
}

void EventLoop::_eventfd_message_callback(uint64_t value) {
    _eventfd_triggered = true;
}

void EventLoop::_timerfd_message_callback(uint64_t value) {
    _timerfd_triggered = true;
}

int64_t EventLoop::_alarm_advancing_threshold = 3;

} // namespace xubinh_server