#ifndef __XUBINH_SERVER_UTIL_ALLOCATOR
#define __XUBINH_SERVER_UTIL_ALLOCATOR

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <set>
#include <vector>

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

    static constexpr const size_t get_number_of_slabs_per_chunk() noexcept {
        return _NUMBER_OF_SLABS_PER_CHUNK;
    }

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

    template <typename AnotherSlabType>
    friend class ThreadLocalStaticSlabAllocator;

public:
    constexpr SimpleSlabAllocator() noexcept = default;

    // do nothing; each instance is independent
    template <typename U>
    constexpr SimpleSlabAllocator(const SimpleSlabAllocator<U> &)
        : SimpleSlabAllocator() {
    }

    ~SimpleSlabAllocator() noexcept override {

        for (auto raw_memory_buffer : _allocated_raw_memory_buffers) {
            ::free(raw_memory_buffer);
        }
    }

    SlabType *allocate(size_t n) override {
        if (n != 1) {
            throw std::bad_alloc();
        }

        if (!_linked_list_of_free_slabs) {
            _allocate_one_chunk();
        }

        auto chosen_slab =
            reinterpret_cast<SlabType *>(_linked_list_of_free_slabs);

        _linked_list_of_free_slabs = _linked_list_of_free_slabs->next;

        return chosen_slab;
    }

    void deallocate(SlabType *slab, size_t n) noexcept override {
        if (n != 1) {
            ::fprintf(stderr, "nultiple slabs are returned at once\n");

            ::abort();
        }

        reinterpret_cast<LinkedListNode *>(slab)->next =
            _linked_list_of_free_slabs;

        _linked_list_of_free_slabs = reinterpret_cast<LinkedListNode *>(slab);
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

        for (size_t i = 0; i < _Base::_NUMBER_OF_SLABS_PER_CHUNK; i++) {
            current_node->next = _linked_list_of_free_slabs;

            _linked_list_of_free_slabs = current_node;

            current_node = reinterpret_cast<LinkedListNode *>(
                reinterpret_cast<SlabType *>(current_node) + 1
            );
        }
    }

    LinkedListNode *_linked_list_of_free_slabs{nullptr};
    std::vector<void *> _allocated_raw_memory_buffers;
};

template <typename SlabType>
class LockFreeSlabAllocator : public SlabAllocatorBase<SlabType> {
private:
    using _Base = SlabAllocatorBase<SlabType>;

    using LinkedListNode = typename _Base::LinkedListNode;

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
                _linked_list_of_free_slabs.load(std::memory_order_relaxed);

            // if saw an empty list, allocates a new chunk and returns the
            // preserved slab directly
            if (!old_head) {
                auto preserved_slab = _allocate_one_chunk();

                return preserved_slab;
            }
        } while (!_linked_list_of_free_slabs.compare_exchange_weak(
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
                _linked_list_of_free_slabs.load(std::memory_order_relaxed);

            head->next = old_head;
        } while (!_linked_list_of_free_slabs.compare_exchange_weak(
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

        if (_Base::_NUMBER_OF_SLABS_PER_CHUNK == 1) {
            return reinterpret_cast<SlabType *>(new_chunk);
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
                old_head =
                    _linked_list_of_free_slabs.load(std::memory_order_relaxed);

                tail->next = old_head;
            } while (!_linked_list_of_free_slabs.compare_exchange_weak(
                old_head,
                head,
                std::memory_order_relaxed,
                std::memory_order_relaxed
            ));
        }

        auto preserved_slab = reinterpret_cast<SlabType *>(head) + 1;

        return preserved_slab;
    }

    std::atomic<LinkedListNode *> _linked_list_of_free_slabs{nullptr};
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

// static member initailization
template <typename SlabType, SlabAllocatorLockType LOCK_TYPE>
typename StaticSlabAllocator<SlabType, LOCK_TYPE>::SlabAllocatorType
    StaticSlabAllocator<SlabType, LOCK_TYPE>::_slab_allocator;

// allocate and deallocate exactly one object each time
//
// - static: combines thread-local pool with a central memory pool for
// performance
template <typename SlabType>
class ThreadLocalStaticSlabAllocator {
private:
    using LinkedListNode =
        typename SimpleSlabAllocator<SlabType>::LinkedListNode;

    struct FreeChunkNode {
        LinkedListNode *chunk_head{nullptr};
        LinkedListNode *chunk_tail{nullptr};
    };

public:
    using value_type = SlabType;

    template <typename AnotherSlabType>
    struct rebind {
        using other = ThreadLocalStaticSlabAllocator<AnotherSlabType>;
    };

    constexpr ThreadLocalStaticSlabAllocator() = default;

    // do nothing; each instance is independent
    template <typename AnotherSlabType>
    constexpr ThreadLocalStaticSlabAllocator(const ThreadLocalStaticSlabAllocator<
                                             AnotherSlabType> &) {
    }

    SlabType *allocate(size_t n) {
        if (_number_of_free_slabs == 0) {
            {
                std::lock_guard<std::mutex> lock(_mutex);

                if (!_free_chunks.empty()) {
                    auto free_chunk_node = _free_chunks.back();

                    _free_chunks.pop_back();

                    free_chunk_node.chunk_tail->next =
                        _slab_allocator._linked_list_of_free_slabs;

                    _slab_allocator._linked_list_of_free_slabs =
                        free_chunk_node.chunk_head;
                }

                else {
                    _cut_down_threshold += _NUMBER_OF_SLABS_PER_CHUNK;
                }
            }

            // a new chunk is always allocated, either from the central pool or
            // directly from the local pool below
            _number_of_free_slabs += _NUMBER_OF_SLABS_PER_CHUNK;
        }

        auto slabs = _slab_allocator.allocate(n);

        _number_of_free_slabs -= n;

        return slabs;
    }

    void deallocate(SlabType *slab, size_t n) noexcept {
        _slab_allocator.deallocate(slab, n);

        _number_of_free_slabs += n;

        if (_number_of_free_slabs >= _cut_down_threshold) {
            LinkedListNode *chunk_head =
                _slab_allocator._linked_list_of_free_slabs;
            LinkedListNode *chunk_tail = chunk_head;

            for (int i = 0; i < _NUMBER_OF_SLABS_PER_CHUNK - 1; i++) {
                chunk_tail = chunk_tail->next;
            }

            _slab_allocator._linked_list_of_free_slabs = chunk_tail->next;

            chunk_tail->next = nullptr;

            FreeChunkNode new_node;

            new_node.chunk_head = chunk_head;
            new_node.chunk_tail = chunk_tail;

            {
                std::lock_guard<std::mutex> lock(_mutex);

                _free_chunks.push_back(new_node);
            }

            _number_of_free_slabs -= _NUMBER_OF_SLABS_PER_CHUNK;
        }
    }

    template <typename... Args>
    void construct(SlabType *slab, Args &&...args) {
        ::new (static_cast<void *>(slab)) SlabType(std::forward<Args>(args)...);
    }

    void destroy(SlabType *slab) noexcept {
        slab->~SlabType();
    }

    bool operator==(const ThreadLocalStaticSlabAllocator &other
    ) const noexcept {
        return true;
    }

    bool operator!=(const ThreadLocalStaticSlabAllocator &other
    ) const noexcept {
        return false;
    }

private:
    static constexpr const size_t _NUMBER_OF_SLABS_PER_CHUNK =
        SimpleSlabAllocator<SlabType>::get_number_of_slabs_per_chunk();

    // local pool
    static thread_local SimpleSlabAllocator<SlabType> _slab_allocator;
    static thread_local size_t _number_of_free_slabs;
    static thread_local size_t _cut_down_threshold;

    // central pool
    static std::vector<FreeChunkNode> _free_chunks;
    static std::mutex _mutex;
};

template <typename SlabType>
thread_local SimpleSlabAllocator<SlabType>
    ThreadLocalStaticSlabAllocator<SlabType>::_slab_allocator;

template <typename SlabType>
thread_local size_t
    ThreadLocalStaticSlabAllocator<SlabType>::_number_of_free_slabs{0};

template <typename SlabType>
thread_local size_t
    ThreadLocalStaticSlabAllocator<SlabType>::_cut_down_threshold{
        _NUMBER_OF_SLABS_PER_CHUNK};

template <typename SlabType>
std::vector<typename ThreadLocalStaticSlabAllocator<SlabType>::FreeChunkNode>
    ThreadLocalStaticSlabAllocator<SlabType>::_free_chunks;

template <typename SlabType>
std::mutex ThreadLocalStaticSlabAllocator<SlabType>::_mutex;

} // namespace util

} // namespace xubinh_server

#endif