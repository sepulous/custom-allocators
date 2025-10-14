/*
    The linear allocator (aka "bump allocator") is the simplest possible allocator.

    Allocation is performed in amortized O(1) time.

    Only one buffer is utilized, which does not grow dynamically, and must be manually resized by the user.

    The linear allocator is often conflated with the arena allocator, but the latter is actually a higher-level system
    which grows dynamically.
*/

#pragma once

#include <cstdlib>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cassert>

#define DEFAULT_ALIGNMENT alignof(max_align_t)

class LinearAllocator
{
    private:
        size_t m_offset;
        size_t m_capacity;
        unsigned char *m_buffer;

    public:
        LinearAllocator(size_t capacity);
        ~LinearAllocator();
        void *alloc(size_t size, size_t alignment = DEFAULT_ALIGNMENT);
        void resize(size_t capacity);
        void free();
};

LinearAllocator::LinearAllocator(size_t capacity)
{
    m_offset = 0;
    m_capacity = capacity;
    m_buffer = static_cast<unsigned char*>(malloc(capacity));
}

LinearAllocator::~LinearAllocator()
{
    std::free(m_buffer);
}

void *LinearAllocator::alloc(size_t size, size_t alignment)
{
    assert((alignment & (alignment - 1)) == 0); // Alignment must be a power of two
    size_t corrected_offset = (m_offset + alignment - 1) & ~(alignment - 1);
    if (size <= m_capacity - corrected_offset)
    {
        m_offset = corrected_offset + size;
        return &m_buffer[corrected_offset];
    }
    return nullptr; // Out of space
}

void LinearAllocator::resize(size_t capacity)
{
    if (capacity <= m_capacity)
        return;

    unsigned char *new_buffer = static_cast<unsigned char *>(malloc(capacity));
    memcpy(new_buffer, m_buffer, m_offset);
    std::free(m_buffer);
    m_buffer = new_buffer;
    m_capacity = capacity;
}

void LinearAllocator::free()
{
    m_offset = 0;
}
