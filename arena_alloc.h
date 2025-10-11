/*
    The arena allocator is a generalization of the linear allocator that can grow dynamically.

    Allocation is performed in O(1) time using only arithmetic.

    In this implementation, dynamic growth is handled by creating new blocks. In some implementations,
    this is instead handled by creating a larger block and copying the data over.

    This implementation supports packing all of the data into a contiguous buffer. To do this in O(n)
    time, rather than O(n^2), with n being the number of blocks, the total size is tracked, which,
    of course, adds overhead. Additionally, if alloc() is used instead of alloc_noalign(), the
    packed data will include any padding used to align the addresses, potentially wasting space.
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
    unsigned char *buffer;
    size_t offset;
    size_t capacity;
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
        void free();
        void *pack(size_t*);
};

ArenaAllocator::ArenaAllocator(size_t initial_capacity)
{
    ArenaBlock *block = (ArenaBlock *)malloc(sizeof(ArenaBlock));
    block->next = nullptr;
    block->buffer = (unsigned char *)malloc(initial_capacity);
    block->offset = 0;
    block->capacity = initial_capacity;
    m_head = block;
    m_current = block;
    m_total_size = 0;
}

ArenaAllocator::~ArenaAllocator()
{
    ArenaBlock *block = m_head;
    while (block != nullptr)
    {
        std::free(block->buffer);
        block = block->next;
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

    if (size <= m_current->capacity - corrected_offset)
    {
        m_total_size += corrected_offset + size - m_current->offset; // add size + offset shift
        m_current->offset = corrected_offset + size;
        return &m_current->buffer[corrected_offset];
    }
    else if (m_current->next != nullptr)
    {
        if (m_current->next->capacity >= size) // Next block is large enough; use it
        {
            m_current = m_current->next;
            m_current->offset += size;
            m_total_size += size;
            return m_current->buffer; // free() was called, so we're at the beginning
        }
        else // Next block is too small; create new one
        {
            ArenaBlock *new_block = (ArenaBlock *)malloc(sizeof(ArenaBlock));
            new_block->next = m_current->next;
            new_block->buffer = (unsigned char *)malloc(size);
            new_block->offset = 0;
            new_block->capacity = size;
            m_current->next = new_block;
            m_current = new_block;
            new_block->offset += size;
            m_total_size += size;
            return new_block->buffer;
        }
    }
    else // Create new block
    {
        ArenaBlock *new_block = (ArenaBlock *)malloc(sizeof(ArenaBlock));
        size_t new_capacity = (size_t)std::max(m_current->capacity * 1.5, (double)size);
        new_block->next = nullptr;
        new_block->buffer = (unsigned char *)malloc(new_capacity);
        new_block->offset = 0;
        new_block->capacity = new_capacity;
        m_current->next = new_block;
        m_current = new_block;
        m_current->offset += size;
        m_total_size += size;
        return new_block->buffer;
    }
}

void *ArenaAllocator::alloc_noalign(size_t size)
{
    if (size <= m_current->capacity - m_current->offset)
    {
        m_total_size += size;
        m_current->offset += size;
        return &m_current->buffer[m_current->offset - size];
    }
    else if (m_current->next != nullptr)
    {
        if (m_current->next->capacity >= size) // Next block is large enough; use it
        {
            m_current = m_current->next;
            m_current->offset += size;
            m_total_size += size;
            return m_current->buffer; // free() was called, so we're at the beginning
        }
        else // Next block is too small; create new one
        {
            ArenaBlock *new_block = (ArenaBlock *)malloc(sizeof(ArenaBlock));
            new_block->next = m_current->next;
            new_block->buffer = (unsigned char *)malloc(size);
            new_block->offset = 0;
            new_block->capacity = size;
            m_current->next = new_block;
            m_current = new_block;
            new_block->offset += size;
            m_total_size += size;
            return new_block->buffer;
        }
    }
    else // Create new block
    {
        ArenaBlock *new_block = (ArenaBlock *)malloc(sizeof(ArenaBlock));
        size_t new_capacity = (size_t)std::max(m_current->capacity * 1.5, (double)size);
        new_block->next = nullptr;
        new_block->buffer = (unsigned char *)malloc(new_capacity);
        new_block->offset = 0;
        new_block->capacity = new_capacity;
        m_current->next = new_block;
        m_current = new_block;
        m_current->offset += size;
        m_total_size += size;
        return new_block->buffer;
    }
}

void ArenaAllocator::free()
{
    ArenaBlock *block = m_head;
    while (block != nullptr)
    {
        block->offset = 0;
        block = block->next;
    }
    m_current = m_head;
    m_total_size = 0;
}

void *ArenaAllocator::pack(size_t *packed_size)
{
    unsigned char *packed_buffer = (unsigned char *)malloc(m_total_size);
    *packed_size = m_total_size;

    ArenaBlock *block = m_head;
    unsigned char *packed_buffer_ptr = packed_buffer;
    while (block != nullptr)
    {
        memcpy(packed_buffer_ptr, block->buffer, block->offset);
        packed_buffer_ptr += block->offset;
        block = block->next;
    }

    return packed_buffer;
}
