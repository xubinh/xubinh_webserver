#include "util/slab_allocator.h"

namespace xubinh_server {

namespace util {

char *StaticSimpleThreadLocalStringSlabAllocator::allocate(size_t n) {
    if (__builtin_expect(n == 0, false)) {
        throw std::bad_alloc();
    }

    if (n > 2048) {
        return reinterpret_cast<char *>(::malloc(n)); // no alignment
    }

    auto power_of_2 =
        alignment::get_next_power_of_2(std::max(n, static_cast<size_t>(32)));

    auto chosen_slab = _linked_lists_of_free_slabs[power_of_2];

    if (!chosen_slab) {
        chosen_slab = _allocate_one_chunk(power_of_2);
    }

    _linked_lists_of_free_slabs[power_of_2] = chosen_slab->next;

    return reinterpret_cast<char *>(chosen_slab);
}

void StaticSimpleThreadLocalStringSlabAllocator::deallocate(
    char *slab, size_t n
) noexcept {
    if (__builtin_expect(n == 0, false)) {
        ::fprintf(
            stderr,
            "@xubinh_server::util::"
            "StaticSimpleThreadLocalStringSlabAllocator::deallocate: "
            "zero slab size\n"
        );

        ::abort();
    }

    if (n > 2048) {
        ::free(slab);

        return;
    }

    auto power_of_2 =
        alignment::get_next_power_of_2(std::max(n, static_cast<size_t>(32)));

    reinterpret_cast<LinkedListNode *>(slab)->next =
        _linked_lists_of_free_slabs[power_of_2];

    _linked_lists_of_free_slabs[power_of_2] =
        reinterpret_cast<LinkedListNode *>(slab);
}

StaticSimpleThreadLocalStringSlabAllocator::LinkedListNode *
StaticSimpleThreadLocalStringSlabAllocator::_allocate_one_chunk(int power_of_2
) noexcept {
    size_t slab_size = (1 << power_of_2);

    // (power_of_2, number_of_slabs) ~ (5, 2^(12-5)), ..., (11, 2^(12-11)) ==>
    // number_of_slabs = 2^(12-power_of_2)
    size_t number_of_slabs = (0x00001000 >> power_of_2);

    void *new_chunk = alignment::aalloc(4096, 4096);

    _allocated_chunks.push_back(new_chunk);

    LinkedListNode *current_node = nullptr;
    LinkedListNode *next_node = reinterpret_cast<LinkedListNode *>(new_chunk);

    for (size_t i = 0; i < number_of_slabs; i++) {
        next_node->next = current_node;

        current_node = next_node;

        next_node = reinterpret_cast<LinkedListNode *>(
            reinterpret_cast<char *>(next_node) + slab_size
        );
    }

    return current_node;
}

// initialization of static members
thread_local StaticSimpleThreadLocalStringSlabAllocator::LinkedListNode *
    StaticSimpleThreadLocalStringSlabAllocator::_linked_lists_of_free_slabs[12]{
        nullptr};
thread_local StaticSimpleThreadLocalStringSlabAllocator::ChunkManager
    StaticSimpleThreadLocalStringSlabAllocator::_allocated_chunks;

} // namespace util

} // namespace xubinh_server

namespace std {

namespace __cxx11 {

// explicit instantiation
template class basic_string<
    char,
    char_traits<char>,
    xubinh_server::util::StaticSimpleThreadLocalStringSlabAllocator>;

} // namespace __cxx11

} // namespace std