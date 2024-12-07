#include "event_loop.h"
#include "util/current_thread.h"

namespace xubinh_server {

template class util::BlockingQueue<EventLoop::FunctorType>;

EventLoop::EventLoop()
    : _eventfd(this), _functor_blocking_queue(_FUNCTOR_QUEUE_CAPACITY),
      _timerfd(this), _owner_thread_tid(util::current_thread::get_tid()) {

    _eventfd.register_read_event_callback(
        std::bind(_read_event_callback_for_eventfd, this)
    );

    _eventfd.enable_read_event();

    _timerfd.register_read_event_callback(
        std::bind(_read_event_callback_for_timerfd, this)
    );

    _timerfd.enable_read_event();
}

EventLoop::~EventLoop() {
    _eventfd.enable_all_event();

    _timerfd.enable_all_event();
}

void EventLoop::loop() {
    while (true) {
        // check for external stop signal
        if (_need_stop.load(std::memory_order_acquire)) {
            break;
        }

        auto event_dispatchers =
            _event_poller.poll_for_active_events_of_all_fds();

        for (auto event_dispatcher_ptr : event_dispatchers) {
            event_dispatcher_ptr->dispatch_active_events();
        }

        if (_eventfd_triggered) {
            const auto &queued_functors = _functor_blocking_queue.pop_all();

            for (auto &functor : queued_functors) {
                functor();
            }

            _eventfd_triggered = false;
        }

        if (_timerfd_triggered) {
            TimePoint current_time_point;

            _expire_all_timers_before_or_at_given_time_point_and_update_alarm(
                current_time_point
            );

            _timerfd_triggered = false;
        }
    }

    // clean up is done inside destructor
}

void EventLoop::run(FunctorType functor) {
    if (_is_in_owner_thread()) {
        functor();
    }

    else {
        _leave_to_owner_thread(std::move(functor));
    }
}

TimerIdentifier EventLoop::run_at_time_point(
    const TimePoint &time_point,
    const TimeInterval &repetition_time_interval,
    int number_of_repetitions,
    FunctorType functor
) {
    Timer *timer_ptr = new Timer(
        time_point,
        repetition_time_interval,
        number_of_repetitions,
        std::move(functor)
    );

    run(std::bind(_add_a_timer_and_update_alarm, this, timer_ptr));

    return TimerIdentifier{timer_ptr};
}

TimerIdentifier EventLoop::run_after_time_interval(
    const TimeInterval &time_interval,
    const TimeInterval &repetition_time_interval,
    int number_of_repetitions,
    FunctorType functor
) {
    TimePoint time_point = (TimePoint() += time_interval);

    const TimerIdentifier &timer_identifier = run_at_time_point(
        time_point,
        repetition_time_interval,
        number_of_repetitions,
        std::move(functor)
    );

    return timer_identifier;
}

void EventLoop::cancel_a_timer(const TimerIdentifier &timer_identifier) {
    const Timer *timer_ptr = timer_identifier._timer_ptr;

    run(std::bind(_cancel_a_timer_and_update_alarm, this, timer_ptr));
}

void EventLoop::ask_to_stop() {
    // a release fence that comes after a store essentially makes the store
    // become a part of the memory ordering, i.e. store first and then do the
    // dummy wake-ups
    //
    // [TODO]: make sure this is correct
    _need_stop.store(true, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_release);

    const int NUMBER_OF_DUMMY_WAKE_UPS = 3;

    // do a bunch of dummy wake-ups
    for (int i = 0; i < NUMBER_OF_DUMMY_WAKE_UPS; i++) {
        _wake_up_this_loop();
    }
}

void EventLoop::_add_a_timer_and_update_alarm(const Timer *timer_ptr) {
    const TimePoint &time_point = timer_ptr->get_expiration_time_point();

    const TimePoint &earliest_expiration_time_point_before_insertion =
        _timer_container.get_earliest_expiration_time_point();

    _timer_container.insert_one(timer_ptr);

    // newly inserted timer advanced the expiration
    if (time_point < earliest_expiration_time_point_before_insertion) {
        _set_alarm_at_time_point(time_point);
    }
}

void EventLoop::_cancel_a_timer_and_update_alarm(const Timer *timer_ptr) {
    bool timer_expired = !_timer_container.remove_one(timer_ptr);

    if (timer_expired) {
        return;
    }

    const TimePoint &earliest_expiration_time_point_after_removal =
        _timer_container.get_earliest_expiration_time_point();

    // expiration delayed after removal
    if (timer_ptr->get_expiration_time_point()
        < earliest_expiration_time_point_after_removal) {

        // if there are still timers inside the container
        if (earliest_expiration_time_point_after_removal < TimePoint::FOREVER) {
            _set_alarm_at_time_point(
                earliest_expiration_time_point_after_removal
            );
        }

        else {
            _cancel_alarm();
        }
    }

    delete timer_ptr;
}

void EventLoop::
    _expire_all_timers_before_or_at_given_time_point_and_update_alarm(
        const TimePoint &time_point
    ) {
    std::vector<const Timer *> expired_timers =
        _timer_container.move_out_before_or_at(time_point);

    std::vector<const Timer *> timers_that_are_still_valid;

    for (const Timer *timer_ptr : expired_timers) {
        auto temp_mutable_timer_ptr = const_cast<Timer *>(timer_ptr);

        if (temp_mutable_timer_ptr->expire_until(time_point)) {
            timers_that_are_still_valid.push_back(timer_ptr);
        }

        else {
            delete timer_ptr;
        }
    }

    _timer_container.insert_all(timers_that_are_still_valid);

    const TimePoint &earliest_expiration_time_point_after_expiration =
        _timer_container.get_earliest_expiration_time_point();

    if (earliest_expiration_time_point_after_expiration < TimePoint::FOREVER) {
        _set_alarm_at_time_point(earliest_expiration_time_point_after_expiration
        );
    }
}

// for async-handling of eventfd notifications
void EventLoop::_read_event_callback_for_eventfd() {
    auto value = _eventfd.retrieve_the_sum();

    if (value) {
        _eventfd_triggered = true;
    }
}

// for async-handling of timerfd notifications
void EventLoop::_read_event_callback_for_timerfd() {
    auto value = _timerfd.retrieve_the_number_of_expirations();

    if (value) {
        _timerfd_triggered = true;
    }
}

} // namespace xubinh_server