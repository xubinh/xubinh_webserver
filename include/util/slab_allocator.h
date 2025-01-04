#ifndef __XUBINH_SERVER_UTIL_ALLOCATOR
#define __XUBINH_SERVER_UTIL_ALLOCATOR

#include <atomic>
#include <cstdio>
#include <mutex>
#include <vector>

#include "util/alignment.h"

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

    // [TODO]: implement copy
    using propagate_on_container_copy_assignment = std::false_type;
    SimpleSlabAllocator(const SimpleSlabAllocator &) = delete;
    SimpleSlabAllocator &operator=(const SimpleSlabAllocator &) = delete;

    // [TODO]: implement move
    using propagate_on_container_move_assignment = std::false_type;
    SimpleSlabAllocator(SimpleSlabAllocator &&) = delete;
    SimpleSlabAllocator &operator=(SimpleSlabAllocator &&) = delete;

    ~SimpleSlabAllocator() noexcept {
        for (auto chunk : _allocated_chunks) {
            ::free(chunk);
        }
    }

    // [TODO]: implement swap
    using propagate_on_container_swap = std::false_type;

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
            ::fprintf(
                stderr,
                "@xubinh_server::util::SimpleSlabAllocator::deallocate: "
                "multiple slabs are returned at once\n"
            );

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

    friend bool
    operator==(const SimpleSlabAllocator &, const SimpleSlabAllocator &) noexcept {
        return false;
    }

    friend bool
    operator!=(const SimpleSlabAllocator &, const SimpleSlabAllocator &) noexcept {
        return true;
    }

private:
    void _allocate_one_chunk() noexcept {
        size_t chunk_size = _SLAB_WIDTH * _NUMBER_OF_SLABS_PER_CHUNK;

        void *new_chunk = alignment::aalloc(_SLAB_ALIGNMENT, chunk_size);

        _allocated_chunks.push_back(new_chunk);

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

    std::vector<void *> _allocated_chunks;
};

// non-static semi lock-free multi-threaded allocator
template <typename SlabType>
class SemiLockFreeSlabAllocator : public SlabAllocatorBase<SlabType> {
private:
    using _Base = SlabAllocatorBase<SlabType>;

    using typename _Base::LinkedListNode;

public:
    using typename _Base::value_type;

    SemiLockFreeSlabAllocator() noexcept = default;

    // do nothing; each instance is independent
    template <typename U>
    SemiLockFreeSlabAllocator(const SemiLockFreeSlabAllocator<U> &) noexcept
        : SemiLockFreeSlabAllocator() {
    }

    // [TODO]: implement copy
    using propagate_on_container_copy_assignment = std::false_type;
    SemiLockFreeSlabAllocator(const SemiLockFreeSlabAllocator &) = delete;
    SemiLockFreeSlabAllocator &
    operator=(const SemiLockFreeSlabAllocator &) = delete;

    // [TODO]: implement move
    using propagate_on_container_move_assignment = std::false_type;
    SemiLockFreeSlabAllocator(SemiLockFreeSlabAllocator &&) = delete;
    SemiLockFreeSlabAllocator &operator=(SemiLockFreeSlabAllocator &&) = delete;

    ~SemiLockFreeSlabAllocator() noexcept {
        {
            std::lock_guard<std::mutex> lock(_mutex);

            if (_allocated_chunks.empty()) {
                return;
            }

            for (auto chunk : _allocated_chunks) {
                ::free(chunk);
            }
        }
    }

    // [TODO]: implement swap
    using propagate_on_container_swap = std::false_type;

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
            ::fprintf(
                stderr,
                "@xubinh_server::util::SemiLockFreeSlabAllocator::deallocate: "
                "multiple slabs are returned at once\n"
            );

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

    friend bool
    operator==(const SemiLockFreeSlabAllocator &, const SemiLockFreeSlabAllocator &) noexcept {
        return false;
    }

    friend bool
    operator!=(const SemiLockFreeSlabAllocator &, const SemiLockFreeSlabAllocator &) noexcept {
        return true;
    }

private:
    SlabType *_allocate_one_chunk() noexcept {
        size_t chunk_size = _SLAB_WIDTH * _NUMBER_OF_SLABS_PER_CHUNK;

        void *new_chunk = alignment::aalloc(_SLAB_ALIGNMENT, chunk_size);

        {
            std::lock_guard<std::mutex> lock(_mutex);

            _allocated_chunks.push_back(new_chunk);
        }

        if (__builtin_expect(_NUMBER_OF_SLABS_PER_CHUNK == 1, false)) {
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

    std::vector<void *> _allocated_chunks;
    std::mutex _mutex;
};

// static simple single-threaded allocator
template <typename SlabType>
class StaticSimpleSlabAllocator : public SlabAllocatorBase<SlabType> {
private:
    using _Base = SlabAllocatorBase<SlabType>;

    using typename _Base::LinkedListNode;

    struct ChunkManager {
        ChunkManager() noexcept = default;

        ChunkManager(const ChunkManager &) = delete;
        ChunkManager &operator=(const ChunkManager &) = delete;

        ~ChunkManager() noexcept {
            for (auto chunk : _allocated_chunks) {
                ::free(chunk);
            }
        }

        void push_back(void *chunk) {
            _allocated_chunks.push_back(chunk);
        }

        std::vector<void *> _allocated_chunks;
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
            ::fprintf(
                stderr,
                "@xubinh_server::util::StaticSimpleSlabAllocator::deallocate: "
                "multiple slabs are returned at once\n"
            );

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

    friend bool
    operator==(const StaticSimpleSlabAllocator &, const StaticSimpleSlabAllocator &) noexcept {
        return true;
    }

    friend bool
    operator!=(const StaticSimpleSlabAllocator &, const StaticSimpleSlabAllocator &) noexcept {
        return false;
    }

private:
    void _allocate_one_chunk() noexcept {
        size_t chunk_size = _SLAB_WIDTH * _NUMBER_OF_SLABS_PER_CHUNK;

        void *new_chunk = alignment::aalloc(_SLAB_ALIGNMENT, chunk_size);

        _allocated_chunks.push_back(new_chunk);

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
    static ChunkManager _allocated_chunks;
};

// initialization of static members
template <typename SlabType>
typename StaticSimpleSlabAllocator<SlabType>::LinkedListNode
    *StaticSimpleSlabAllocator<SlabType>::_linked_list_of_free_slabs{nullptr};
template <typename SlabType>
typename StaticSimpleSlabAllocator<SlabType>::ChunkManager
    StaticSimpleSlabAllocator<SlabType>::_allocated_chunks;

// static semi lock-free multi-threaded allocator
template <typename SlabType>
class StaticSemiLockFreeSlabAllocator : public SlabAllocatorBase<SlabType> {
private:
    using _Base = SlabAllocatorBase<SlabType>;

    using typename _Base::LinkedListNode;

    struct ChunkManager {
        ChunkManager() noexcept = default;

        ChunkManager(const ChunkManager &) = delete;
        ChunkManager &operator=(const ChunkManager &) = delete;

        ~ChunkManager() noexcept {
            for (auto chunk : _allocated_chunks) {
                ::free(chunk);
            }
        }

        void push_back(void *chunk) {
            {
                std::lock_guard<std::mutex> lock(_mutex);

                _allocated_chunks.push_back(chunk);
            }
        }

        std::vector<void *> _allocated_chunks;
        std::mutex _mutex;
    };

public:
    using typename _Base::value_type;

    constexpr StaticSemiLockFreeSlabAllocator() noexcept = default;

    // do nothing; each instance is independent
    template <typename U>
    constexpr StaticSemiLockFreeSlabAllocator(const StaticSemiLockFreeSlabAllocator<
                                              U> &) noexcept
        : StaticSemiLockFreeSlabAllocator() {
    }

    ~StaticSemiLockFreeSlabAllocator() noexcept = default;

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
            ::fprintf(
                stderr,
                "@xubinh_server::util::StaticSemiLockFreeSlabAllocator::"
                "deallocate: multiple slabs are returned at once\n"
            );

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

    friend bool
    operator==(const StaticSemiLockFreeSlabAllocator &, const StaticSemiLockFreeSlabAllocator &) noexcept {
        return true;
    }

    friend bool
    operator!=(const StaticSemiLockFreeSlabAllocator &, const StaticSemiLockFreeSlabAllocator &) noexcept {
        return false;
    }

private:
    SlabType *_allocate_one_chunk() noexcept {
        size_t chunk_size = _SLAB_WIDTH * _NUMBER_OF_SLABS_PER_CHUNK;

        void *new_chunk = alignment::aalloc(_SLAB_ALIGNMENT, chunk_size);

        _allocated_chunks.push_back(new_chunk);

        if (__builtin_expect(_NUMBER_OF_SLABS_PER_CHUNK == 1, false)) {
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
    static ChunkManager _allocated_chunks;
};

// initialization of static members
template <typename SlabType>
std::atomic<
    typename StaticSemiLockFreeSlabAllocator<SlabType>::LinkedListNode *>
    StaticSemiLockFreeSlabAllocator<SlabType>::_linked_list_of_free_slabs{
        nullptr};
template <typename SlabType>
typename StaticSemiLockFreeSlabAllocator<SlabType>::ChunkManager
    StaticSemiLockFreeSlabAllocator<SlabType>::_allocated_chunks;

// static multi-threaded allocator that combines thread-local pools with a
// central memory pool
template <typename SlabType>
class StaticThreadLocalSlabAllocator : public SlabAllocatorBase<SlabType> {
private:
    using _Base = SlabAllocatorBase<SlabType>;

    using typename _Base::LinkedListNode;

    struct ChunkManager {
        ChunkManager() noexcept = default;

        ChunkManager(const ChunkManager &) = delete;
        ChunkManager &operator=(const ChunkManager &) = delete;

        ~ChunkManager() noexcept {
            for (auto chunk : _allocated_chunks) {
                ::free(chunk);
            }
        }

        void push_back(void *chunk) {
            _allocated_chunks.push_back(chunk);
        }

        std::vector<void *> _allocated_chunks;
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
            ::fprintf(
                stderr,
                "@xubinh_server::util::StaticThreadLocalSlabAllocator::"
                "deallocate: multiple slabs are returned at once\n"
            );

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

    friend bool
    operator==(const StaticThreadLocalSlabAllocator &, const StaticThreadLocalSlabAllocator &) noexcept {
        return true;
    }

    friend bool
    operator!=(const StaticThreadLocalSlabAllocator &, const StaticThreadLocalSlabAllocator &) noexcept {
        return false;
    }

private:
    void _allocate_one_chunk() noexcept {
        size_t chunk_size = _SLAB_WIDTH * _NUMBER_OF_SLABS_PER_CHUNK;

        void *new_chunk = alignment::aalloc(_SLAB_ALIGNMENT, chunk_size);

        _allocated_chunks.push_back(new_chunk);

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
    static thread_local ChunkManager _allocated_chunks;
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
thread_local typename StaticThreadLocalSlabAllocator<SlabType>::ChunkManager
    StaticThreadLocalSlabAllocator<SlabType>::_allocated_chunks;
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