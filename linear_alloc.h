/*
    The linear allocator (aka "bump allocator") is the simplest possible allocator.

    Allocation is performed in O(1) time using only arithmetic.

    Only one buffer is utilized, which does not grow dynamically, and must be manually resized by the user.

    The linear allocator is often conflated with the arena allocator, but the latter is actually a higher-level system
    which grows dynamically by either managing multiple buffers, or copying the existing buffer into a larger one.
*/

#pragma once

#include <cstdlib>
#include <cstdint>
#include <cassert>

#define ALIGNMENT sizeof(void *)

class LinearAllocator
{
    private:
        unsigned char *m_buffer;
        size_t m_offset;
        size_t m_capacity;

    public:
        LinearAllocator(size_t);
        ~LinearAllocator();
        void *alloc(size_t);
        void *alloc_noalign(size_t);
        void resize(size_t);
        void free();
        void *get_buffer();
        size_t get_offset();
        size_t get_capacity();
};

LinearAllocator::LinearAllocator(size_t capacity)
{
    m_buffer = (unsigned char *)malloc(capacity);
    m_offset = 0;
    m_capacity = capacity;
}

LinearAllocator::~LinearAllocator()
{
    std::free(m_buffer);
}

void *LinearAllocator::alloc(size_t size)
{
    assert((ALIGNMENT & (ALIGNMENT-1)) == 0); // Alignment must be a power of two

    // Alignment correction
    auto boundary_gap = ((uintptr_t)m_buffer + m_offset) & (ALIGNMENT-1); // A % B == A & (B-1)  if B is a power of two (and A >= 0)
    size_t corrected_offset = m_offset;
    if (boundary_gap != 0)
        corrected_offset += ALIGNMENT - boundary_gap;

    if (size <= m_capacity - corrected_offset)
    {
        m_offset = corrected_offset + size;
        return &m_buffer[corrected_offset];
    }
    return nullptr; // Out of space
}

void *LinearAllocator::alloc_noalign(size_t size)
{
    if (size <= m_capacity - m_offset)
    {
        m_offset += size;
        return &m_buffer[m_offset - size];
    }
    return nullptr; // Out of space
}

void LinearAllocator::resize(size_t capacity)
{
    if (capacity <= m_capacity)
        return;

    auto new_buffer = (unsigned char *)malloc(capacity);
    memcpy(new_buffer, m_buffer, m_capacity - m_offset + 1);
    std::free(m_buffer);
    m_buffer = new_buffer;
    m_capacity = capacity;
}

void LinearAllocator::free()
{
    m_offset = 0;
}

void *LinearAllocator::get_buffer()
{
    return m_buffer;
}

size_t LinearAllocator::get_offset()
{
    return m_offset;
}

size_t LinearAllocator::get_capacity()
{
    return m_capacity;
}
