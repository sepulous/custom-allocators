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
    the total size is tracked across allocations, which, of course, adds overhead. Also, if alloc() is used instead
    of alloc_noalign(), the packed data will include any padding used to align the addresses, potentially wasting space.
*/

#pragma once

#include <algorithm>
#include <cstdlib>
#include <cstdint>
#include <cassert>

#define ALIGNMENT sizeof(void *)

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
        ArenaBlock *m_head;
        ArenaBlock *m_current;
        size_t m_total_size; // So packing is O(n) instead of O(n^2)

    public:
        ArenaAllocator(size_t);
        ~ArenaAllocator();
        void *alloc(size_t);
        void *alloc_noalign(size_t);
        void reset();
        void free();
        void *pack(size_t*);
};

ArenaAllocator::ArenaAllocator(size_t initial_capacity)
{
    ArenaBlock *block = static_cast<ArenaBlock *>(malloc(sizeof(ArenaBlock) + initial_capacity));
    block->next = nullptr;
    block->offset = 0;
    block->capacity = initial_capacity;
    block->buffer = reinterpret_cast<unsigned char *>(block + 1);
    m_head = m_current = block;
    m_total_size = 0;
}

ArenaAllocator::~ArenaAllocator()
{
    ArenaBlock *block = m_head;
    while (block)
    {
        ArenaBlock *next = block->next;
        std::free(block);
        block = next;
    }
}

void *ArenaAllocator::alloc(size_t size)
{
    assert((ALIGNMENT & (ALIGNMENT-1)) == 0); // Alignment must be a power of two

    // Alignment correction
    auto boundary_gap = ((uintptr_t)m_current->buffer + m_current->offset) & (ALIGNMENT-1); // A % B == A & (B-1)  if B is a power of two (and A >= 0)
    size_t corrected_offset = m_current->offset;
    if (boundary_gap != 0)
        corrected_offset += ALIGNMENT - boundary_gap;

    if (size > m_current->capacity - corrected_offset)
    {
        ArenaBlock *new_block = static_cast<ArenaBlock *>(malloc(sizeof(ArenaBlock) + size));
        new_block->next = m_current->next;
        new_block->offset = 0;
        new_block->capacity = static_cast<size_t>(std::max(m_current->capacity * 1.5, (double)size));
        new_block->buffer = reinterpret_cast<unsigned char *>(new_block + 1);
        m_current->next = new_block;
        m_current = new_block;
        m_current->offset += size;
        m_total_size += size;
        return new_block + 1;
    }
    else
    {
        m_total_size += size + corrected_offset - m_current->offset; // += size + offset shift
        m_current->offset = corrected_offset + size;
        return &(m_current->buffer[corrected_offset - size]);
    }
}

void *ArenaAllocator::alloc_noalign(size_t size)
{
    if (size > m_current->capacity - m_current->offset)
    {
        ArenaBlock *new_block = static_cast<ArenaBlock *>(malloc(sizeof(ArenaBlock) + size));
        new_block->next = m_current->next;
        new_block->offset = 0;
        new_block->capacity = static_cast<size_t>(std::max(m_current->capacity * 1.5, static_cast<double>(size)));
        new_block->buffer = reinterpret_cast<unsigned char *>(new_block + 1);
        m_current->next = new_block;
        m_current = new_block;
    }

    m_total_size += size;
    m_current->offset += size;
    return &(m_current->buffer[m_current->offset - size]);
}

void ArenaAllocator::reset()
{
    ArenaBlock *block = m_head;
    while (block)
    {
        block->offset = 0;
        block = block->next;
    }
    m_current = m_head;
    m_total_size = 0;
}

void ArenaAllocator::free()
{
    ArenaBlock *block = m_head->next;
    while (block)
    {
        ArenaBlock *next = block->next;
        std::free(block);
        block = next;
    }
    m_head->offset = 0;
    m_head->next = nullptr;
    m_current = m_head;
    m_total_size = 0;
}

void *ArenaAllocator::pack(size_t *packed_size)
{
    if (m_total_size == 0)
        return nullptr;

    unsigned char *packed_buffer = static_cast<unsigned char *>(malloc(m_total_size));
    *packed_size = m_total_size;

    ArenaBlock *block = m_head;
    unsigned char *packed_buffer_ptr = packed_buffer;
    while (block)
    {
        memcpy(packed_buffer_ptr, block->buffer, block->offset);
        packed_buffer_ptr += block->offset;
        block = block->next;
    }

    return packed_buffer;
}
