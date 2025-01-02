#ifndef __XUBINH_SERVER_UTIL_ALLOCATOR
#define __XUBINH_SERVER_UTIL_ALLOCATOR

// #define __XUBINH_SERVER_UTIL_ALLOCATOR_DEBUG

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <vector>

#ifdef __XUBINH_SERVER_UTIL_ALLOCATOR_DEBUG
#include "util/time_point.h"
#include "util/type_name.h"
#endif

namespace xubinh_server {

namespace util {

template <typename SlabType>
class SlabAllocatorBase {
public:
    static_assert(
        sizeof(SlabType) > 0, "allocator should not be used for an empty class"
    );

    static_assert(
        sizeof(SlabType) >= 8, "size of type must be at least 8 bytes"
    );

    static_assert(
        sizeof(SlabType) % 8 == 0, "size of type must be multiples of 8 bytes"
    );

    using value_type = SlabType;

    virtual ~SlabAllocatorBase() noexcept = default;

    virtual SlabType *allocate(size_t n) = 0;

    virtual void deallocate(SlabType *slab, size_t n) noexcept = 0;

protected:
    struct LinkedListNode {
        LinkedListNode *next;
    };

    static constexpr const size_t _SLAB_WIDTH = sizeof(SlabType);

    static constexpr const size_t _SLAB_ALIGNMENT = alignof(SlabType);

    static constexpr const size_t _NUMBER_OF_SLABS_PER_CHUNK = 4000;
};

// allocate and deallocate exactly one object each time
//
// - not thread-safe
// - not static: each instance forms an allocator and has its own memory pool
template <typename SlabType>
class SimpleSlabAllocator : public SlabAllocatorBase<SlabType> {
private:
    using _Base = SlabAllocatorBase<SlabType>;

    using LinkedListNode = typename _Base::LinkedListNode;

    static_assert(_Base::_NUMBER_OF_SLABS_PER_CHUNK >= 1);

public:
    constexpr SimpleSlabAllocator() noexcept = default;

    // do nothing; each instance is independent
    template <typename U>
    constexpr SimpleSlabAllocator(const SimpleSlabAllocator<U> &)
        : SimpleSlabAllocator() {
    }

    ~SimpleSlabAllocator() noexcept override {
#ifdef __XUBINH_SERVER_UTIL_ALLOCATOR_DEBUG
        if (_number_of_available_slabs
            != _allocated_raw_memory_buffers.size()
                   * _Base::_NUMBER_OF_SLABS_PER_CHUNK) {

            ::fprintf(
                stderr,
                "slab allocator destructed before all slabs are returned, "
                "number of missing slabs: %d, slab type: %s, time stamp: %s\n",
                static_cast<int>(
                    _allocated_raw_memory_buffers.size()
                        * _Base::_NUMBER_OF_SLABS_PER_CHUNK
                    - _number_of_available_slabs
                ),
                TypeName<SlabType>::get_name().c_str(),
                TimePoint::get_datetime_string(TimePoint::Purpose::PRINTING)
                    .c_str()
            );

            ::fprintf(stderr, "addresses of slabs:\n");

            for (auto address : _addresses_of_allocated_slabs) {
                ::fprintf(stderr, "%lx", address);

                if (sizeof(SlabType) == sizeof(TcpConnectSocketfd) + 16) {
                    ::fprintf(
                        stderr,
                        ", id: %ld\n",
                        reinterpret_cast<TcpConnectSocketfd *>(
                            reinterpret_cast<char *>(address) + 16
                        )
                            ->get_id()
                    );
                }

                else {
                    ::fprintf(stderr, "\n");
                }
            }
        }
#endif

        for (auto raw_memory_buffer : _allocated_raw_memory_buffers) {
            ::free(raw_memory_buffer);
        }
    }

    SlabType *allocate(size_t n) override {
        if (n != 1) {
            throw std::bad_alloc();
        }

        if (!_linked_list_of_available_slabs) {
            _allocate_one_chunk();
        }

#ifdef __XUBINH_SERVER_UTIL_ALLOCATOR_DEBUG
        if (_linked_list_of_available_slabs
            && !_check_if_is_in_range(_linked_list_of_available_slabs)) {
            ::fprintf(
                stderr,
                "not in range, address: %lx\n",
                reinterpret_cast<uint64_t>(_linked_list_of_available_slabs)
            );

            ::abort();
        }
#endif

        auto chosen_slab =
            reinterpret_cast<SlabType *>(_linked_list_of_available_slabs);

        _linked_list_of_available_slabs = _linked_list_of_available_slabs->next;

#ifdef __XUBINH_SERVER_UTIL_ALLOCATOR_DEBUG
        _number_of_available_slabs--;

        _addresses_of_allocated_slabs.insert(
            reinterpret_cast<uint64_t>(chosen_slab)
        );

        if (!_check_if_is_in_range(chosen_slab)) {
            ::fprintf(
                stderr,
                "not in range, address: %lx\n",
                reinterpret_cast<uint64_t>(chosen_slab)
            );

            ::abort();
        }

        if (_linked_list_of_available_slabs
            && !_check_if_is_in_range(_linked_list_of_available_slabs)) {
            ::fprintf(
                stderr,
                "not in range, address: %lx\n",
                reinterpret_cast<uint64_t>(_linked_list_of_available_slabs)
            );

            ::abort();
        }
#endif

        return chosen_slab;
    }

    void deallocate(SlabType *slab, size_t n) noexcept override {
        if (n != 1) {
            ::fprintf(stderr, "nultiple slabs are returned at once\n");

            ::abort();
        }

#ifdef __XUBINH_SERVER_UTIL_ALLOCATOR_DEBUG
        _addresses_of_allocated_slabs.erase(reinterpret_cast<uint64_t>(slab));

        if (!_check_if_is_in_range(slab)) {
            ::fprintf(
                stderr,
                "not in range, address: %lx\n",
                reinterpret_cast<uint64_t>(slab)
            );

            ::abort();
        }

        if (_linked_list_of_available_slabs
            && !_check_if_is_in_range(_linked_list_of_available_slabs)) {
            ::fprintf(
                stderr,
                "not in range, address: %lx\n",
                reinterpret_cast<uint64_t>(_linked_list_of_available_slabs)
            );

            ::abort();
        }

        _number_of_available_slabs++;
#endif

        reinterpret_cast<LinkedListNode *>(slab)->next =
            _linked_list_of_available_slabs;

        _linked_list_of_available_slabs =
            reinterpret_cast<LinkedListNode *>(slab);
    }

    template <typename... Args>
    void construct(SlabType *slab, Args &&...args) {
        ::new (static_cast<void *>(slab)) SlabType(std::forward<Args>(args)...);
    }

    void destroy(SlabType *slab) noexcept {
        slab->~SlabType();
    }

private:
    void _allocate_one_chunk() {
        size_t chunk_size =
            _Base::_SLAB_WIDTH * _Base::_NUMBER_OF_SLABS_PER_CHUNK;

        size_t raw_memory_buffer_size =
            chunk_size + (_Base::_SLAB_ALIGNMENT - 1);

        void *new_raw_memory_buffer = ::malloc(raw_memory_buffer_size);

        if (!new_raw_memory_buffer) {
            ::fprintf(stderr, "memory allocation failed\n");

            ::abort();
        }

        _allocated_raw_memory_buffers.push_back(new_raw_memory_buffer);

        auto new_chunk = new_raw_memory_buffer;

        if (!std::align(
                _Base::_SLAB_ALIGNMENT,
                chunk_size,
                new_chunk,
                raw_memory_buffer_size
            )) {
            ::fprintf(stderr, "alignment failed\n");

            ::abort();
        }

        LinkedListNode *current_node =
            reinterpret_cast<LinkedListNode *>(new_chunk);

#ifdef __XUBINH_SERVER_UTIL_ALLOCATOR_DEBUG
        auto first_node = current_node;
#endif

        for (size_t i = 0; i < _Base::_NUMBER_OF_SLABS_PER_CHUNK; i++) {
            current_node->next = _linked_list_of_available_slabs;

            _linked_list_of_available_slabs = current_node;

            current_node = reinterpret_cast<LinkedListNode *>(
                reinterpret_cast<SlabType *>(current_node) + 1
            );
        }

#ifdef __XUBINH_SERVER_UTIL_ALLOCATOR_DEBUG
        _number_of_available_slabs += _Base::_NUMBER_OF_SLABS_PER_CHUNK;

        ::fprintf(
            stderr,
            "new raw memory buffer address: %lx | "
            "new chunk address: %lx | "
            "first node: %lx | "
            "last node: %lx | "
            "slab size: %ld | "
            "slab alignment: %ld | "
            "number of supplement: %ld | "
            "chunk size: %ld | "
            "time stamp: %s | "
            "slab type: %s\n",
            reinterpret_cast<uint64_t>(new_raw_memory_buffer),
            reinterpret_cast<uint64_t>(new_chunk),
            reinterpret_cast<uint64_t>(first_node),
            reinterpret_cast<uint64_t>(current_node),
            _Base::_SLAB_WIDTH,
            _Base::_SLAB_ALIGNMENT,
            _Base::_NUMBER_OF_SLABS_PER_CHUNK,
            chunk_size,
            TimePoint::get_datetime_string(TimePoint::Purpose::PRINTING)
                .c_str(),
            TypeName<SlabType>::get_name().c_str()
        );
#endif
    }

#ifdef __XUBINH_SERVER_UTIL_ALLOCATOR_DEBUG
    bool _check_if_is_in_range(void *slab) {
        bool is_in_range = false;

        for (auto raw_memory_buffer : _allocated_raw_memory_buffers) {
            if (reinterpret_cast<uint64_t>(slab)
                    >= reinterpret_cast<uint64_t>(raw_memory_buffer)
                && (reinterpret_cast<uint64_t>(slab)
                    - reinterpret_cast<uint64_t>(raw_memory_buffer))
                           / _Base::_SLAB_WIDTH
                       < _Base::_NUMBER_OF_SLABS_PER_CHUNK + 5) {
                is_in_range = true;

                break;
            }
        }

        return is_in_range;
    }
#endif

    LinkedListNode *_linked_list_of_available_slabs{nullptr};
    std::vector<void *> _allocated_raw_memory_buffers;

#ifdef __XUBINH_SERVER_UTIL_ALLOCATOR_DEBUG
    size_t _number_of_available_slabs{0};
    std::set<uint64_t> _addresses_of_allocated_slabs;
#endif
};

template <typename SlabType>
class LockFreeSlabAllocator : public SlabAllocatorBase<SlabType> {
private:
    using _Base = SlabAllocatorBase<SlabType>;

    using LinkedListNode = typename _Base::LinkedListNode;

    static_assert(_Base::_NUMBER_OF_SLABS_PER_CHUNK >= 2);

    struct RawMemoryBufferAddressLinkedListNode {
        RawMemoryBufferAddressLinkedListNode *next{nullptr};
        void *raw_memory_buffer_address{nullptr};
    };

public:
    constexpr LockFreeSlabAllocator() = default;

    // do nothing; each instance is independent
    template <typename U>
    constexpr LockFreeSlabAllocator(const LockFreeSlabAllocator<U> &)
        : LockFreeSlabAllocator() {
    }

    ~LockFreeSlabAllocator() noexcept override {
        RawMemoryBufferAddressLinkedListNode *old_head;

        do {
            old_head = _linked_list_of_allocated_raw_memory_buffers.load(
                std::memory_order_relaxed
            );

            if (!old_head) {
                return;
            }

            if (_linked_list_of_allocated_raw_memory_buffers
                    .compare_exchange_weak(
                        old_head,
                        old_head->next,
                        std::memory_order_relaxed,
                        std::memory_order_relaxed
                    )) {

                ::free(old_head->raw_memory_buffer_address);

                delete old_head;
            }
        } while (true);
    }

    SlabType *allocate(size_t n) override {
        if (n != 1) {
            throw std::bad_alloc();
        }

        LinkedListNode *old_head;

        do {
            old_head =
                _linked_list_of_available_slabs.load(std::memory_order_relaxed);

            // if saw an empty list, allocates a new chunk and returns the
            // preserved slab directly
            if (!old_head) {
                auto preserved_slab = _allocate_one_chunk();

                return preserved_slab;
            }
        } while (!_linked_list_of_available_slabs.compare_exchange_weak(
            old_head,
            old_head->next,
            std::memory_order_relaxed,
            std::memory_order_relaxed
        ));

        auto chosen_slab = reinterpret_cast<SlabType *>(old_head);

        return chosen_slab;
    }

    void deallocate(SlabType *slab, size_t n) noexcept override {
        if (n != 1) {
            ::fprintf(stderr, "nultiple slabs are returned at once\n");

            ::abort();
        }

        auto head = reinterpret_cast<LinkedListNode *>(slab);

        LinkedListNode *old_head;

        do {
            old_head =
                _linked_list_of_available_slabs.load(std::memory_order_relaxed);

            head->next = old_head;
        } while (!_linked_list_of_available_slabs.compare_exchange_weak(
            old_head, head, std::memory_order_relaxed, std::memory_order_relaxed
        ));
    }

    template <typename... Args>
    void construct(SlabType *slab, Args &&...args) {
        ::new (static_cast<void *>(slab)) SlabType(std::forward<Args>(args)...);
    }

    void destroy(SlabType *slab) noexcept {
        slab->~SlabType();
    }

private:
    SlabType *_allocate_one_chunk() {
        size_t chunk_size =
            _Base::_SLAB_WIDTH * _Base::_NUMBER_OF_SLABS_PER_CHUNK;

        size_t raw_memory_buffer_size =
            chunk_size + (_Base::_SLAB_ALIGNMENT - 1);

        void *new_raw_memory_buffer = ::malloc(raw_memory_buffer_size);

        if (!new_raw_memory_buffer) {
            ::fprintf(stderr, "memory allocation failed\n");

            ::abort();
        }

        auto new_raw_memory_buffer_address_node =
            new RawMemoryBufferAddressLinkedListNode;

        new_raw_memory_buffer_address_node->raw_memory_buffer_address =
            new_raw_memory_buffer;

        {
            RawMemoryBufferAddressLinkedListNode *old_head;

            do {
                old_head = _linked_list_of_allocated_raw_memory_buffers.load(
                    std::memory_order_relaxed
                );
                new_raw_memory_buffer_address_node->next = old_head;
            } while (!_linked_list_of_allocated_raw_memory_buffers
                          .compare_exchange_weak(
                              old_head,
                              new_raw_memory_buffer_address_node,
                              std::memory_order_relaxed,
                              std::memory_order_relaxed
                          ));
        }

        auto new_chunk = new_raw_memory_buffer;

        if (!std::align(
                _Base::_SLAB_ALIGNMENT,
                chunk_size,
                new_chunk,
                raw_memory_buffer_size
            )) {
            ::fprintf(stderr, "alignment failed\n");

            ::abort();
        }

        LinkedListNode *current_node =
            reinterpret_cast<LinkedListNode *>(new_chunk);

        auto previous_node = current_node;

        auto tail = current_node;

        // skip the tail node, and also preserve the head node
        for (size_t i = 1; i < _Base::_NUMBER_OF_SLABS_PER_CHUNK - 1; i++) {
            current_node = reinterpret_cast<LinkedListNode *>(
                reinterpret_cast<SlabType *>(previous_node) + 1
            );

            current_node->next = previous_node;

            previous_node = current_node;
        }

        auto head = current_node;

        {
            LinkedListNode *old_head;

            do {
                old_head = _linked_list_of_available_slabs.load(
                    std::memory_order_relaxed
                );

                tail->next = old_head;
            } while (!_linked_list_of_available_slabs.compare_exchange_weak(
                old_head,
                head,
                std::memory_order_relaxed,
                std::memory_order_relaxed
            ));
        }

        auto preserved_slab = reinterpret_cast<SlabType *>(head) + 1;

        return preserved_slab;
    }

    std::atomic<LinkedListNode *> _linked_list_of_available_slabs{nullptr};
    std::atomic<RawMemoryBufferAddressLinkedListNode *>
        _linked_list_of_allocated_raw_memory_buffers{nullptr};
};

enum class SlabAllocatorLockType { SIMPLE = 0, LOCK_FREE };

template <SlabAllocatorLockType LOCK_TYPE, typename SlabType>
struct __SlabAllocatorTypeDispatcher;

template <typename SlabType>
struct __SlabAllocatorTypeDispatcher<SlabAllocatorLockType::SIMPLE, SlabType> {
    using type = SimpleSlabAllocator<SlabType>;
};

template <typename SlabType>
struct __SlabAllocatorTypeDispatcher<
    SlabAllocatorLockType::LOCK_FREE,
    SlabType> {
    using type = LockFreeSlabAllocator<SlabType>;
};

// allocate and deallocate exactly one object each time
//
// - not thread-safe
// - static: all instances use one same static memory pool
template <
    typename SlabType,
    SlabAllocatorLockType LOCK_TYPE = SlabAllocatorLockType::LOCK_FREE>
class StaticSlabAllocator {
private:
    using SlabAllocatorType =
        typename __SlabAllocatorTypeDispatcher<LOCK_TYPE, SlabType>::type;

public:
    using value_type = SlabType;

    template <typename AnotherSlabType>
    struct rebind {
        using other = StaticSlabAllocator<AnotherSlabType, LOCK_TYPE>;
    };

    constexpr StaticSlabAllocator() = default;

    // do nothing; each instance is independent
    template <typename AnotherSlabType>
    constexpr StaticSlabAllocator(const StaticSlabAllocator<
                                  AnotherSlabType,
                                  LOCK_TYPE> &) {
    }

    SlabType *allocate(size_t n) {
        return _slab_allocator.allocate(n);
    }

    void deallocate(SlabType *slab, size_t n) noexcept {
        _slab_allocator.deallocate(slab, n);
    }

    template <typename... Args>
    void construct(SlabType *slab, Args &&...args) {
        ::new (static_cast<void *>(slab)) SlabType(std::forward<Args>(args)...);
    }

    void destroy(SlabType *slab) noexcept {
        slab->~SlabType();
    }

    bool operator==(const StaticSlabAllocator &other) const noexcept {
        return true;
    }

    bool operator!=(const StaticSlabAllocator &other) const noexcept {
        return false;
    }

private:
    static SlabAllocatorType _slab_allocator;
};

template <typename SlabType, SlabAllocatorLockType LOCK_TYPE>
typename StaticSlabAllocator<SlabType, LOCK_TYPE>::SlabAllocatorType
    StaticSlabAllocator<SlabType, LOCK_TYPE>::_slab_allocator;

} // namespace util

} // namespace xubinh_server

#endif