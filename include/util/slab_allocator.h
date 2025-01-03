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

// base for "slab allocator", i.e. allocates and deallocates exactly one object
// each time
//
// - non-static allocator: each instance of this class is an
// independent allocator and has its own memory pool
// - static allocator: all instances are consider the same and point to one
// single static (or even thread local) memory pool
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

protected:
    struct LinkedListNode {
        LinkedListNode *next;
    };
};

// non-static simple single-threaded allocator
template <typename SlabType>
class SimpleSlabAllocator : public SlabAllocatorBase<SlabType> {
private:
    using _Base = SlabAllocatorBase<SlabType>;

    using typename _Base::LinkedListNode;

public:
    using typename _Base::value_type;

    SimpleSlabAllocator() noexcept = default;

    // do nothing; each instance is independent
    template <typename U>
    SimpleSlabAllocator(const SimpleSlabAllocator<U> &) noexcept
        : SimpleSlabAllocator() {
    }

    ~SimpleSlabAllocator() noexcept {
        for (auto raw_memory_buffer : _allocated_raw_memory_buffers) {
            ::free(raw_memory_buffer);
        }
    }

    SlabType *allocate(size_t n) {
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

    void deallocate(SlabType *slab, size_t n) noexcept {
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
    void _allocate_one_chunk() noexcept {
        size_t chunk_size = _SLAB_WIDTH * _NUMBER_OF_SLABS_PER_CHUNK;

        size_t raw_memory_buffer_size = chunk_size + (_SLAB_ALIGNMENT - 1);

        void *new_raw_memory_buffer = ::malloc(raw_memory_buffer_size);

        if (!new_raw_memory_buffer) {
            ::fprintf(stderr, "memory allocation failed\n");

            ::abort();
        }

        _allocated_raw_memory_buffers.push_back(new_raw_memory_buffer);

        auto new_chunk = new_raw_memory_buffer;

        if (!std::align(
                _SLAB_ALIGNMENT, chunk_size, new_chunk, raw_memory_buffer_size
            )) {

            ::fprintf(stderr, "alignment failed\n");

            ::abort();
        }

        LinkedListNode *current_node =
            reinterpret_cast<LinkedListNode *>(new_chunk);

        for (size_t i = 0; i < _NUMBER_OF_SLABS_PER_CHUNK; i++) {
            current_node->next = _linked_list_of_free_slabs;

            _linked_list_of_free_slabs = current_node;

            current_node = reinterpret_cast<LinkedListNode *>(
                reinterpret_cast<SlabType *>(current_node) + 1
            );
        }
    }

    static constexpr const size_t _SLAB_WIDTH = sizeof(SlabType);

    static constexpr const size_t _SLAB_ALIGNMENT = alignof(SlabType);

    static constexpr const size_t _NUMBER_OF_SLABS_PER_CHUNK = 4000;

    LinkedListNode *_linked_list_of_free_slabs{nullptr};

    std::vector<void *> _allocated_raw_memory_buffers;
};

// non-static lock-free multi-threaded allocator
template <typename SlabType>
class LockFreeSlabAllocator : public SlabAllocatorBase<SlabType> {
private:
    using _Base = SlabAllocatorBase<SlabType>;

    using typename _Base::LinkedListNode;

public:
    using typename _Base::value_type;

    LockFreeSlabAllocator() noexcept = default;

    // do nothing; each instance is independent
    template <typename U>
    LockFreeSlabAllocator(const LockFreeSlabAllocator<U> &) noexcept
        : LockFreeSlabAllocator() {
    }

    ~LockFreeSlabAllocator() noexcept {
        {
            std::lock_guard<std::mutex> lock(_mutex);

            if (_allocated_raw_memory_buffers.empty()) {
                return;
            }

            for (auto raw_memory_buffer : _allocated_raw_memory_buffers) {
                ::free(raw_memory_buffer);
            }
        }
    }

    SlabType *allocate(size_t n) {
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

    void deallocate(SlabType *slab, size_t n) noexcept {
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
    SlabType *_allocate_one_chunk() noexcept {
        size_t chunk_size = _SLAB_WIDTH * _NUMBER_OF_SLABS_PER_CHUNK;

        size_t raw_memory_buffer_size = chunk_size + (_SLAB_ALIGNMENT - 1);

        void *new_raw_memory_buffer = ::malloc(raw_memory_buffer_size);

        if (!new_raw_memory_buffer) {
            ::fprintf(stderr, "memory allocation failed\n");

            ::abort();
        }

        {
            std::lock_guard<std::mutex> lock(_mutex);

            _allocated_raw_memory_buffers.push_back(new_raw_memory_buffer);
        }

        auto new_chunk = new_raw_memory_buffer;

        if (!std::align(
                _SLAB_ALIGNMENT, chunk_size, new_chunk, raw_memory_buffer_size
            )) {

            ::fprintf(stderr, "alignment failed\n");

            ::abort();
        }

        if (_NUMBER_OF_SLABS_PER_CHUNK == 1) {
            auto preserved_slab = reinterpret_cast<SlabType *>(new_chunk);

            return preserved_slab;
        }

        LinkedListNode *current_node =
            reinterpret_cast<LinkedListNode *>(new_chunk);

        auto previous_node = current_node;

        auto tail = current_node;

        // skip the tail node, and also preserve the head node
        for (size_t i = 1; i < _NUMBER_OF_SLABS_PER_CHUNK - 1; i++) {
            current_node = reinterpret_cast<LinkedListNode *>(
                reinterpret_cast<SlabType *>(previous_node) + 1
            );

            current_node->next = previous_node;

            previous_node = current_node;
        }

        auto head = current_node;

        LinkedListNode *old_head;

        do {
            old_head =
                _linked_list_of_free_slabs.load(std::memory_order_relaxed);

            tail->next = old_head;
        } while (!_linked_list_of_free_slabs.compare_exchange_weak(
            old_head, head, std::memory_order_relaxed, std::memory_order_relaxed
        ));

        auto preserved_slab = reinterpret_cast<SlabType *>(head) + 1;

        return preserved_slab;
    }

    static constexpr const size_t _SLAB_WIDTH = sizeof(SlabType);

    static constexpr const size_t _SLAB_ALIGNMENT = alignof(SlabType);

    static constexpr const size_t _NUMBER_OF_SLABS_PER_CHUNK = 4000;

    std::atomic<LinkedListNode *> _linked_list_of_free_slabs{nullptr};

    std::vector<void *> _allocated_raw_memory_buffers;
    std::mutex _mutex;
};

// static simple single-threaded allocator
template <typename SlabType>
class StaticSimpleSlabAllocator : public SlabAllocatorBase<SlabType> {
private:
    using _Base = SlabAllocatorBase<SlabType>;

    using typename _Base::LinkedListNode;

    struct RawMemoryBufferManager {
        RawMemoryBufferManager() noexcept = default;

        RawMemoryBufferManager(const RawMemoryBufferManager &) = delete;
        RawMemoryBufferManager &
        operator=(const RawMemoryBufferManager &) = delete;

        ~RawMemoryBufferManager() noexcept {
            for (auto raw_memory_buffer : _allocated_raw_memory_buffers) {
                ::free(raw_memory_buffer);
            }
        }

        void push_back(void *raw_memory_buffer) noexcept {
            _allocated_raw_memory_buffers.push_back(raw_memory_buffer);
        }

        std::vector<void *> _allocated_raw_memory_buffers;
    };

public:
    using typename _Base::value_type;

    constexpr StaticSimpleSlabAllocator() noexcept = default;

    // do nothing; each instance is independent
    template <typename U>
    constexpr StaticSimpleSlabAllocator(const StaticSimpleSlabAllocator<U>
                                            &) noexcept
        : StaticSimpleSlabAllocator() {
    }

    ~StaticSimpleSlabAllocator() noexcept = default;

    SlabType *allocate(size_t n) {
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

    void deallocate(SlabType *slab, size_t n) noexcept {
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
    void _allocate_one_chunk() noexcept {
        size_t chunk_size = _SLAB_WIDTH * _NUMBER_OF_SLABS_PER_CHUNK;

        size_t raw_memory_buffer_size = chunk_size + (_SLAB_ALIGNMENT - 1);

        void *new_raw_memory_buffer = ::malloc(raw_memory_buffer_size);

        if (!new_raw_memory_buffer) {
            ::fprintf(stderr, "memory allocation failed\n");

            ::abort();
        }

        _allocated_raw_memory_buffers.push_back(new_raw_memory_buffer);

        auto new_chunk = new_raw_memory_buffer;

        if (!std::align(
                _SLAB_ALIGNMENT, chunk_size, new_chunk, raw_memory_buffer_size
            )) {

            ::fprintf(stderr, "alignment failed\n");

            ::abort();
        }

        LinkedListNode *current_node =
            reinterpret_cast<LinkedListNode *>(new_chunk);

        for (size_t i = 0; i < _NUMBER_OF_SLABS_PER_CHUNK; i++) {
            current_node->next = _linked_list_of_free_slabs;

            _linked_list_of_free_slabs = current_node;

            current_node = reinterpret_cast<LinkedListNode *>(
                reinterpret_cast<SlabType *>(current_node) + 1
            );
        }
    }

    static constexpr const size_t _SLAB_WIDTH = sizeof(SlabType);

    static constexpr const size_t _SLAB_ALIGNMENT = alignof(SlabType);

    static constexpr const size_t _NUMBER_OF_SLABS_PER_CHUNK = 4000;

    static LinkedListNode *_linked_list_of_free_slabs;
    static RawMemoryBufferManager _allocated_raw_memory_buffers;
};

// initialization of static members
template <typename SlabType>
typename StaticSimpleSlabAllocator<SlabType>::LinkedListNode
    *StaticSimpleSlabAllocator<SlabType>::_linked_list_of_free_slabs{nullptr};
template <typename SlabType>
typename StaticSimpleSlabAllocator<SlabType>::RawMemoryBufferManager
    StaticSimpleSlabAllocator<SlabType>::_allocated_raw_memory_buffers;

// static lock-free multi-threaded allocator
template <typename SlabType>
class StaticLockFreeSlabAllocator : public SlabAllocatorBase<SlabType> {
private:
    using _Base = SlabAllocatorBase<SlabType>;

    using typename _Base::LinkedListNode;

    struct RawMemoryBufferManager {
        RawMemoryBufferManager() noexcept = default;

        RawMemoryBufferManager(const RawMemoryBufferManager &) = delete;
        RawMemoryBufferManager &
        operator=(const RawMemoryBufferManager &) = delete;

        ~RawMemoryBufferManager() noexcept {
            for (auto raw_memory_buffer : _allocated_raw_memory_buffers) {
                ::free(raw_memory_buffer);
            }
        }

        void push_back(void *raw_memory_buffer) noexcept {
            {
                std::lock_guard<std::mutex> lock(_mutex);

                _allocated_raw_memory_buffers.push_back(raw_memory_buffer);
            }
        }

        std::vector<void *> _allocated_raw_memory_buffers;
        std::mutex _mutex;
    };

public:
    using typename _Base::value_type;

    constexpr StaticLockFreeSlabAllocator() noexcept = default;

    // do nothing; each instance is independent
    template <typename U>
    constexpr StaticLockFreeSlabAllocator(const StaticLockFreeSlabAllocator<U>
                                              &) noexcept
        : StaticLockFreeSlabAllocator() {
    }

    ~StaticLockFreeSlabAllocator() noexcept = default;

    SlabType *allocate(size_t n) {
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

    void deallocate(SlabType *slab, size_t n) noexcept {
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
    SlabType *_allocate_one_chunk() noexcept {
        size_t chunk_size = _SLAB_WIDTH * _NUMBER_OF_SLABS_PER_CHUNK;

        size_t raw_memory_buffer_size = chunk_size + (_SLAB_ALIGNMENT - 1);

        void *new_raw_memory_buffer = ::malloc(raw_memory_buffer_size);

        if (!new_raw_memory_buffer) {
            ::fprintf(stderr, "memory allocation failed\n");

            ::abort();
        }

        _allocated_raw_memory_buffers.push_back(new_raw_memory_buffer);

        auto new_chunk = new_raw_memory_buffer;

        if (!std::align(
                _SLAB_ALIGNMENT, chunk_size, new_chunk, raw_memory_buffer_size
            )) {

            ::fprintf(stderr, "alignment failed\n");

            ::abort();
        }

        if (_NUMBER_OF_SLABS_PER_CHUNK == 1) {
            auto preserved_slab = reinterpret_cast<SlabType *>(new_chunk);

            return preserved_slab;
        }

        LinkedListNode *current_node =
            reinterpret_cast<LinkedListNode *>(new_chunk);

        auto previous_node = current_node;

        auto tail = current_node;

        // skip the tail node, and also preserve the head node
        for (size_t i = 1; i < _NUMBER_OF_SLABS_PER_CHUNK - 1; i++) {
            current_node = reinterpret_cast<LinkedListNode *>(
                reinterpret_cast<SlabType *>(previous_node) + 1
            );

            current_node->next = previous_node;

            previous_node = current_node;
        }

        auto head = current_node;

        LinkedListNode *old_head;

        do {
            old_head =
                _linked_list_of_free_slabs.load(std::memory_order_relaxed);

            tail->next = old_head;
        } while (!_linked_list_of_free_slabs.compare_exchange_weak(
            old_head, head, std::memory_order_relaxed, std::memory_order_relaxed
        ));

        auto preserved_slab = reinterpret_cast<SlabType *>(head) + 1;

        return preserved_slab;
    }

    static constexpr const size_t _SLAB_WIDTH = sizeof(SlabType);

    static constexpr const size_t _SLAB_ALIGNMENT = alignof(SlabType);

    static constexpr const size_t _NUMBER_OF_SLABS_PER_CHUNK = 4000;

    static std::atomic<LinkedListNode *> _linked_list_of_free_slabs;

    static RawMemoryBufferManager _allocated_raw_memory_buffers;
};

// initialization of static members
template <typename SlabType>
std::atomic<typename StaticLockFreeSlabAllocator<SlabType>::LinkedListNode *>
    StaticLockFreeSlabAllocator<SlabType>::_linked_list_of_free_slabs{nullptr};
template <typename SlabType>
typename StaticLockFreeSlabAllocator<SlabType>::RawMemoryBufferManager
    StaticLockFreeSlabAllocator<SlabType>::_allocated_raw_memory_buffers;

// static multi-threaded allocator that combines thread-local pools with a
// central memory pool
template <typename SlabType>
class StaticThreadLocalSlabAllocator : public SlabAllocatorBase<SlabType> {
private:
    using _Base = SlabAllocatorBase<SlabType>;

    using typename _Base::LinkedListNode;

    struct RawMemoryBufferManager {
        RawMemoryBufferManager() noexcept = default;

        RawMemoryBufferManager(const RawMemoryBufferManager &) = delete;
        RawMemoryBufferManager &
        operator=(const RawMemoryBufferManager &) = delete;

        ~RawMemoryBufferManager() noexcept {
            for (auto raw_memory_buffer : _allocated_raw_memory_buffers) {
                ::free(raw_memory_buffer);
            }
        }

        void push_back(void *raw_memory_buffer) noexcept {
            _allocated_raw_memory_buffers.push_back(raw_memory_buffer);
        }

        std::vector<void *> _allocated_raw_memory_buffers;
    };

    struct FreeChunkNode {
        LinkedListNode *chunk_head{nullptr};
        LinkedListNode *chunk_tail{nullptr};
    };

public:
    using typename _Base::value_type;

    constexpr StaticThreadLocalSlabAllocator() noexcept = default;

    // do nothing; each instance is independent
    template <typename AnotherSlabType>
    constexpr StaticThreadLocalSlabAllocator(const StaticThreadLocalSlabAllocator<
                                             AnotherSlabType> &) noexcept
        : StaticThreadLocalSlabAllocator() {
    }

    SlabType *allocate(size_t n) {
        if (n != 1) {
            throw std::bad_alloc();
        }

        // if free list is empty
        if (_number_of_free_slabs == 0) {
            FreeChunkNode free_chunk_node;

            // try to get a full chunk from the central pool first
            {
                std::lock_guard<std::mutex> lock(_mutex);
                if (!_free_chunks.empty()) {
                    free_chunk_node = _free_chunks.back();
                    _free_chunks.pop_back();
                }
            }

            // if got one from the central pool
            if (free_chunk_node.chunk_head) {
                // mount this chunk on the local free list
                free_chunk_node.chunk_tail->next = _linked_list_of_free_slabs;
                _linked_list_of_free_slabs = free_chunk_node.chunk_head;
            }

            // otherwise allocate a new chunk directly
            else {
                _allocate_one_chunk();
                _cut_off_threshold += _NUMBER_OF_SLABS_PER_CHUNK;
            }

            _number_of_free_slabs += _NUMBER_OF_SLABS_PER_CHUNK;
        }

        // get one slab off the free list and return
        auto chosen_slab =
            reinterpret_cast<SlabType *>(_linked_list_of_free_slabs);
        _linked_list_of_free_slabs = _linked_list_of_free_slabs->next;
        _number_of_free_slabs--;

        return chosen_slab;
    }

    void deallocate(SlabType *slab, size_t n) noexcept {
        if (n != 1) {
            ::fprintf(stderr, "nultiple slabs are returned at once\n");

            ::abort();
        }

        // mount this slab on the free list first
        reinterpret_cast<LinkedListNode *>(slab)->next =
            _linked_list_of_free_slabs;
        _linked_list_of_free_slabs = reinterpret_cast<LinkedListNode *>(slab);
        _number_of_free_slabs++;

        // if free list contains too much slabs that belongs to other thread
        if (_number_of_free_slabs >= _cut_off_threshold) {
            // chunk head is just the first free slab
            LinkedListNode *chunk_head = _linked_list_of_free_slabs;

            // chunk tail needs to be located manually
            LinkedListNode *chunk_tail = chunk_head;
            for (int i = 0; i < _NUMBER_OF_SLABS_PER_CHUNK - 1; i++) {
                chunk_tail = chunk_tail->next;
            }

            // cut this chunk off the free list
            _linked_list_of_free_slabs = chunk_tail->next;
            chunk_tail->next = nullptr;

            // move this chunk back to the central pool for reclamation
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

private:
    void _allocate_one_chunk() noexcept {
        size_t chunk_size = _SLAB_WIDTH * _NUMBER_OF_SLABS_PER_CHUNK;

        size_t raw_memory_buffer_size = chunk_size + (_SLAB_ALIGNMENT - 1);

        void *new_raw_memory_buffer = ::malloc(raw_memory_buffer_size);

        if (!new_raw_memory_buffer) {
            ::fprintf(stderr, "memory allocation failed\n");

            ::abort();
        }

        _allocated_raw_memory_buffers.push_back(new_raw_memory_buffer);

        auto new_chunk = new_raw_memory_buffer;

        if (!std::align(
                _SLAB_ALIGNMENT, chunk_size, new_chunk, raw_memory_buffer_size
            )) {

            ::fprintf(stderr, "alignment failed\n");

            ::abort();
        }

        LinkedListNode *current_node =
            reinterpret_cast<LinkedListNode *>(new_chunk);

        for (size_t i = 0; i < _NUMBER_OF_SLABS_PER_CHUNK; i++) {
            current_node->next = _linked_list_of_free_slabs;

            _linked_list_of_free_slabs = current_node;

            current_node = reinterpret_cast<LinkedListNode *>(
                reinterpret_cast<SlabType *>(current_node) + 1
            );
        }
    }

    static constexpr const size_t _SLAB_WIDTH = sizeof(SlabType);

    static constexpr const size_t _SLAB_ALIGNMENT = alignof(SlabType);

    static constexpr const size_t _NUMBER_OF_SLABS_PER_CHUNK = 4000;

    // local pool
    static thread_local LinkedListNode *_linked_list_of_free_slabs;
    static thread_local size_t _number_of_free_slabs;
    static thread_local RawMemoryBufferManager _allocated_raw_memory_buffers;
    static thread_local size_t _cut_off_threshold;

    // central pool
    static std::vector<FreeChunkNode> _free_chunks;
    static std::mutex _mutex;
};

// initialization of static members
template <typename SlabType>
thread_local typename StaticThreadLocalSlabAllocator<SlabType>::LinkedListNode
    *StaticThreadLocalSlabAllocator<SlabType>::_linked_list_of_free_slabs{
        nullptr};
template <typename SlabType>
thread_local size_t
    StaticThreadLocalSlabAllocator<SlabType>::_number_of_free_slabs{0};
template <typename SlabType>
thread_local
    typename StaticThreadLocalSlabAllocator<SlabType>::RawMemoryBufferManager
        StaticThreadLocalSlabAllocator<SlabType>::_allocated_raw_memory_buffers;
template <typename SlabType>
thread_local size_t
    StaticThreadLocalSlabAllocator<SlabType>::_cut_off_threshold{
        _NUMBER_OF_SLABS_PER_CHUNK};
template <typename SlabType>
std::vector<typename StaticThreadLocalSlabAllocator<SlabType>::FreeChunkNode>
    StaticThreadLocalSlabAllocator<SlabType>::_free_chunks;
template <typename SlabType>
std::mutex StaticThreadLocalSlabAllocator<SlabType>::_mutex;

} // namespace util

} // namespace xubinh_server

#endif