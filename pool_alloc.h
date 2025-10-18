/*
    The pool allocator divides a buffer into fixed-size chunks.

    Allocation and individual frees are performed in O(1) time using a free list stored
    across unused chunks.
*/

#ifndef POOL_ALLOC_H
#define POOL_ALLOC_H

#include <cstdlib>
#include <cstddef>
#include <cassert>

struct FreePoolNode
{
    FreePoolNode *next;
};

class PoolAllocator
{
    private:
        size_t _chunk_count;
        size_t _chunk_size;
        unsigned char *_buffer;
        FreePoolNode *_free_list_head;

    public:
        PoolAllocator(size_t chunk_count, size_t chunk_size);
        PoolAllocator(size_t chunk_count, size_t chunk_size, size_t chunk_alignment);
        ~PoolAllocator();
        void *alloc();
        void free(void *chunk);
        void free_all();
};

PoolAllocator::PoolAllocator(size_t chunk_count, size_t chunk_size)
{
    constexpr size_t DEFAULT_ALIGNMENT = alignof(max_align_t);

    _chunk_count = chunk_count;
    _chunk_size = (chunk_size + DEFAULT_ALIGNMENT - 1) & ~(DEFAULT_ALIGNMENT - 1);
    _buffer = static_cast<unsigned char *>(malloc(chunk_count * _chunk_size));
    free_all(); // Build initial free list
}

PoolAllocator::PoolAllocator(size_t chunk_count, size_t chunk_size, size_t chunk_alignment)
{
    assert((chunk_alignment & (chunk_alignment - 1)) == 0); // Alignment must be a power of two

    _chunk_count = chunk_count;
    _chunk_size = (chunk_size + chunk_alignment - 1) & ~(chunk_alignment - 1);
    _buffer = static_cast<unsigned char *>(malloc(chunk_count * _chunk_size));
    free_all(); // Build initial free list
}

PoolAllocator::~PoolAllocator()
{
    std::free(_buffer);
}

void *PoolAllocator::alloc()
{
    FreePoolNode *free_node = _free_list_head;

    if (!free_node)
        return nullptr;

    _free_list_head = free_node->next;
    return free_node;
}

void PoolAllocator::free(void *chunk)
{
    FreePoolNode *free_node = reinterpret_cast<FreePoolNode *>(chunk);
    free_node->next = _free_list_head;
    _free_list_head = free_node;
}

void PoolAllocator::free_all()
{
    _free_list_head = reinterpret_cast<FreePoolNode *>(_buffer);
    FreePoolNode *current_free_node = _free_list_head;
    for (int i = 0; i < _chunk_count - 1; i++)
    {
        current_free_node->next = reinterpret_cast<FreePoolNode *>(_buffer + (i+1) * _chunk_size);
        current_free_node = current_free_node->next;
    }
    current_free_node->next = nullptr;
}

#endif
