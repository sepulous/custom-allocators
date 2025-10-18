/*
    The arena allocator is a generalization of the linear allocator that can grow dynamically.

    Allocation is performed in amortized O(1) time, assuming the current block has enough space.

    Two functions are provided to "reset" the allocator: reset() and free(). reset() simply resets the offset
    of each block, keeping them in memory. free() actually releases the blocks. Unlike some implementations,
    here, free() only resets the first block and doesn't release it. This was done to simplify the allocation logic;
    basically trading semantic purity for performance. Calling the destructor releases all blocks.

    In this implementation, dynamic growth is handled by creating new blocks (in some implementations,
    this is instead handled by creating a larger block and copying the data over). These new blocks
    are inserted after the current block, which means that if there are already existing blocks after
    the current block (because reset() was called), a memory leak results, which must be managed by
    calling free() eventually.

    This implementation supports packing all of the data into a contiguous buffer. To make packing more efficient,
    the total size is tracked across allocations, which, of course, adds overhead.
*/

#ifndef ARENA_ALLOC_H
#define ARENA_ALLOC_H

#include <algorithm>
#include <cstdlib>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cassert>

struct ArenaBlock
{
    ArenaBlock *next;
    size_t offset;
    size_t capacity;
    unsigned char *buffer;
};

class ArenaAllocator
{
    private:
        ArenaBlock *_head;
        ArenaBlock *_current;
        size_t _total_size; // So packing is O(n) instead of O(n^2)

    public:
        ArenaAllocator(size_t capacity);
        ~ArenaAllocator();
        void *alloc(size_t size);
        void *alloc_align(size_t size, size_t alignment);
        void reset();
        void free();
        void *pack(size_t *packed_size);
};

ArenaAllocator::ArenaAllocator(size_t capacity)
{
    ArenaBlock *block = static_cast<ArenaBlock *>(malloc(sizeof(ArenaBlock) + capacity));
    block->next = nullptr;
    block->offset = 0;
    block->capacity = capacity;
    block->buffer = reinterpret_cast<unsigned char *>(block + 1);
    _head = _current = block;
    _total_size = 0;
}

ArenaAllocator::~ArenaAllocator()
{
    ArenaBlock *block = _head;
    while (block)
    {
        ArenaBlock *next = block->next;
        std::free(block);
        block = next;
    }
}

void *ArenaAllocator::alloc(size_t size)
{
    constexpr size_t DEFAULT_ALIGNMENT = alignof(max_align_t);

    size_t corrected_offset = (_current->offset + DEFAULT_ALIGNMENT - 1) & ~(DEFAULT_ALIGNMENT - 1);
    if (size > _current->capacity - corrected_offset)
    {
        ArenaBlock *new_block = static_cast<ArenaBlock *>(malloc(sizeof(ArenaBlock) + size));
        new_block->next = _current->next;
        new_block->offset = 0;
        new_block->capacity = static_cast<size_t>(std::max(_current->capacity * 1.5, (double)size));
        new_block->buffer = reinterpret_cast<unsigned char *>(new_block + 1);
        _current->next = new_block;
        _current = new_block;
        _current->offset += size;
        _total_size += size;
        return new_block + 1;
    }
    else
    {
        _total_size += size + corrected_offset - _current->offset; // += size + offset shift
        _current->offset = corrected_offset + size;
        return &(_current->buffer[corrected_offset - size]);
    }
}

void *ArenaAllocator::alloc_align(size_t size, size_t alignment)
{
    assert((alignment & (alignment - 1)) == 0); // Alignment must be a power of two

    size_t corrected_offset = (_current->offset + alignment - 1) & ~(alignment - 1);
    if (size > _current->capacity - corrected_offset)
    {
        ArenaBlock *new_block = static_cast<ArenaBlock *>(malloc(sizeof(ArenaBlock) + size));
        new_block->next = _current->next;
        new_block->offset = 0;
        new_block->capacity = static_cast<size_t>(std::max(_current->capacity * 1.5, (double)size));
        new_block->buffer = reinterpret_cast<unsigned char *>(new_block + 1);
        _current->next = new_block;
        _current = new_block;
        _current->offset += size;
        _total_size += size;
        return new_block + 1;
    }
    else
    {
        _total_size += size + corrected_offset - _current->offset; // += size + offset shift
        _current->offset = corrected_offset + size;
        return &(_current->buffer[corrected_offset - size]);
    }
}

void ArenaAllocator::reset()
{
    ArenaBlock *block = _head;
    while (block)
    {
        block->offset = 0;
        block = block->next;
    }
    _current = _head;
    _total_size = 0;
}

void ArenaAllocator::free()
{
    ArenaBlock *block = _head->next;
    while (block)
    {
        ArenaBlock *next = block->next;
        std::free(block);
        block = next;
    }
    _head->offset = 0;
    _head->next = nullptr;
    _current = _head;
    _total_size = 0;
}

void *ArenaAllocator::pack(size_t *packed_size)
{
    if (_total_size == 0)
        return nullptr;

    unsigned char *packed_buffer = static_cast<unsigned char *>(malloc(_total_size));
    *packed_size = _total_size;

    ArenaBlock *block = _head;
    unsigned char *packed_buffer_ptr = packed_buffer;
    while (block)
    {
        memcpy(packed_buffer_ptr, block->buffer, block->offset);
        packed_buffer_ptr += block->offset;
        block = block->next;
    }

    return packed_buffer;
}

#endif
