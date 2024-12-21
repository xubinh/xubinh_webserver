#include "event_loop.h"
#include "util/current_thread.h"

namespace xubinh_server {

EventLoop::EventLoop(
    uint64_t loop_index, size_t number_of_functor_blocking_queues
)
    : _loop_index(loop_index),
      _number_of_functor_blocking_queues(
          std::max(number_of_functor_blocking_queues, static_cast<uint64_t>(1))
      ),
      _functor_blocking_queues(_number_of_functor_blocking_queues),
      _eventfds(_number_of_functor_blocking_queues),
      _timerfd(Timerfd::create_timerfd(0), this),
      _owner_thread_tid(util::current_thread::get_tid()) {

    for (int i = 0; i < _number_of_functor_blocking_queues; i++) {
        _functor_blocking_queues[i].reset(
            new FunctorQueue(_FUNCTOR_QUEUE_CAPACITY)
        );

        _eventfds[i].reset(new Eventfd(Eventfd::create_eventfd(0), this));
        _eventfds[i]->register_eventfd_message_callback(std::bind(
            &EventLoop::_eventfd_message_callback, this, std::placeholders::_1
        ));

        _eventfds[i]->start();
    }

    _timerfd.register_timerfd_message_callback(std::bind(
        &EventLoop::_timerfd_message_callback, this, std::placeholders::_1
    ));

    _timerfd.start();
}

void EventLoop::loop() {
    while (true) {
        // checks for stop signal (either external or from within) and stops
        // only when the polling list is logically empty (which really is not
        // since each event loop always got two fixed fd's being listened on,
        // i.e. a timerfd and an eventfd)
        if (_need_stop.load(std::memory_order_relaxed)
            && _event_poller.size() == 1 + _number_of_functor_blocking_queues) {

            LOG_INFO << "event loop exits, target TID: " << _owner_thread_tid;

            break;
        }

        auto event_dispatchers =
            _event_poller.poll_for_active_events_of_all_fds();

        LOG_DEBUG << "number of dispatchers: "
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

            for (auto &_functor_blocking_queue_ptr : _functor_blocking_queues) {
                const auto &queued_functors =
                    _functor_blocking_queue_ptr->pop_all();

                for (auto &functor : queued_functors) {
                    functor();
                }
            }

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

        LOG_DEBUG << "current size of poller: " << _event_poller.size();
    }

    // clean up is done inside destructor
}

void EventLoop::run(
    FunctorType functor, uint64_t functor_blocking_queue_index
) {
    if (_is_in_owner_thread()) {
        functor();
    }

    else {
        _leave_to_owner_thread(
            std::move(functor), functor_blocking_queue_index
        );
    }
}

TimerIdentifier EventLoop::run_at_time_point(
    const TimePoint &time_point,
    const TimeInterval &repetition_time_interval,
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

    run(std::bind(&EventLoop::_add_a_timer_and_update_alarm, this, timer_ptr),
        functor_blocking_queue_index);

    return TimerIdentifier{timer_ptr};
}

TimerIdentifier EventLoop::run_after_time_interval(
    const TimeInterval &time_interval,
    const TimeInterval &repetition_time_interval,
    int number_of_repetitions,
    FunctorType functor,
    uint64_t functor_blocking_queue_index
) {
    TimePoint time_point = (TimePoint() += time_interval);

    const TimerIdentifier &timer_identifier = run_at_time_point(
        time_point,
        repetition_time_interval,
        number_of_repetitions,
        std::move(functor),
        functor_blocking_queue_index
    );

    return timer_identifier;
}

void EventLoop::cancel_a_timer(const TimerIdentifier &timer_identifier) {
    const Timer *timer_ptr = timer_identifier._timer_ptr;

    run(std::bind(&EventLoop::_cancel_a_timer_and_update_alarm, this, timer_ptr)
    );
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
    _functor_blocking_queues
        [functor_blocking_queue_index % _number_of_functor_blocking_queues]
            ->push(std::move(functor));

    LOG_TRACE << "leave to owner thread, functor queue index: "
              << (functor_blocking_queue_index
                  % _number_of_functor_blocking_queues);

    _wake_up_this_loop(functor_blocking_queue_index);
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

void EventLoop::_expire_and_update_alarm(const TimePoint &time_point) {
    // [NOTE] entering this function means the timer, which is always one-shot,
    // has expired and the event loop is awakened by timerfd to execute this
    // function, so one can assume the timer is clean right now

    std::vector<const Timer *> expired_timers =
        _timer_container.move_out_before_or_at(time_point);

    const TimePoint &exp_after_moving_out =
        _timer_container.get_earliest_expiration_time_point();

    std::vector<const Timer *> timers_that_are_still_valid;

    // the callbacks themself might also add their new timers and update the
    // alarm
    for (const Timer *timer_ptr : expired_timers) {
        auto temp_mutable_timer_ptr = const_cast<Timer *>(timer_ptr);

        if (temp_mutable_timer_ptr->expire_until(time_point)) {
            timers_that_are_still_valid.push_back(timer_ptr);
        }

        else {
            delete timer_ptr;
        }
    }

    if (timers_that_are_still_valid.empty()) {
        return;
    }

    const TimePoint &exp_after_executing_callbacks =
        _timer_container.get_earliest_expiration_time_point();

    bool _new_callbacks_have_been_inserted =
        exp_after_executing_callbacks < exp_after_moving_out;

    _timer_container.insert_all(timers_that_are_still_valid);

    const TimePoint &exp_after_inserting_all =
        _timer_container.get_earliest_expiration_time_point();

    // set the alarm if it is not set inside the callback, or it is set inside
    // the callback but will be advanced by the insertion here
    if (!_new_callbacks_have_been_inserted
        || exp_after_inserting_all < exp_after_executing_callbacks) {
        _set_alarm_at_time_point(exp_after_inserting_all);
    }
}

void EventLoop::_release_all_timers() {
    std::vector<const Timer *> all_timers =
        _timer_container.move_out_before_or_at(TimePoint::FOREVER);

    for (const Timer *timer_ptr : all_timers) {
        delete timer_ptr;
    }
}

void EventLoop::_eventfd_message_callback(uint64_t value) {
    if (value) {
        _eventfd_triggered = true;
    }
}

void EventLoop::_timerfd_message_callback(uint64_t value) {
    if (value) {
        _timerfd_triggered = true;
    }
}

} // namespace xubinh_server