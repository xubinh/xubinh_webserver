#ifndef __XUBINH_SERVER_LOCK_FREE_QUEUE
#define __XUBINH_SERVER_LOCK_FREE_QUEUE

#include <atomic>
#include <memory>

namespace xubinh_server {

namespace util {

// single-producer, single-consumer, and lock-free queue
template <typename T>
class SpscLockFreeQueue {
public:
    class Node {
    public:
#ifdef __USE_LOCK_FREE_QUEUE_WITH_RAW_POINTER
        T *get_value() {
            return _value;
        }
#else
        std::shared_ptr<T> get_value() {
            return _value;
        }
#endif

        Node *get_next_node() {
            return _next.load(std::memory_order_relaxed);
        }

    private:
        friend SpscLockFreeQueue;

        Node() = default;

#ifdef __USE_LOCK_FREE_QUEUE_WITH_RAW_POINTER
        Node(const T &value) : _value(new T(value)) {
        }

        Node(T &&value) : _value(new T(std::move(value))) {
        }

        template <typename... Args>
        Node(Args &&...args) : _value(new T(std::forward<Args>(args)...)) {
        }
#else
        Node(const T &value) : _value(std::make_shared<T>(value)) {
        }

        Node(T &&value) : _value(std::make_shared<T>(std::move(value))) {
        }

        template <typename... Args>
        Node(Args &&...args)
            : _value(std::make_shared<T>(std::forward<Args>(args)...)) {
        }
#endif

        std::atomic<Node *> _next{nullptr};

#ifdef __USE_LOCK_FREE_QUEUE_WITH_RAW_POINTER
        T *_value{nullptr};
#else
        std::shared_ptr<T> _value{nullptr};
#endif
    };

public:
    SpscLockFreeQueue() : _head(new Node), _tail(_head) {
    }

    // not thread-safe
    ~SpscLockFreeQueue() {
        Node *current_node = _head;

        while (auto next_node = current_node->_next.load()) {
#ifdef __USE_LOCK_FREE_QUEUE_WITH_RAW_POINTER
            delete next_node->_value;
#endif
            delete current_node;

            current_node = next_node;
        }

        delete current_node;
    }

    void push(const T &value) {
        Node *new_node = new Node(value);

        _push(new_node);
    }

    void push(T &&value) {
        Node *new_node = new Node(std::move(value));

        _push(new_node);
    }

    template <typename... Args>
    void push(Args &&...args) {
        Node *new_node = new Node(std::forward<Args>(args)...);

        _push(new_node);
    }

#ifdef __USE_LOCK_FREE_QUEUE_WITH_RAW_POINTER
    T *pop() {
        Node *next = _head->_next.load(std::memory_order_acquire);

        if (next == nullptr) {
            return nullptr;
        }

        T *popped_element = next->_value;
        next->_value = nullptr;

        Node *old_head = _head;
        _head = next;

        delete old_head;

        return popped_element;
    }
#else
    std::shared_ptr<T> pop() {
        Node *next = _head->_next.load(std::memory_order_acquire);

        if (next == nullptr) {
            return nullptr;
        }

        std::shared_ptr<T> popped_element = next->_value;
        next->_value.reset();

        Node *old_head = _head;
        _head = next;

        delete old_head;

        return popped_element;
    }
#endif

private:
    void _push(Node *new_node) {
        Node *old_tail = _tail;
        _tail = new_node;
        old_tail->_next.store(new_node, std::memory_order_release);
    }

    Node *_head;
    Node *_tail;
};

} // namespace util

} // namespace xubinh_server

#endif