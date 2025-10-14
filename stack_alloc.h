/*
    The stack allocator is a modified linear allocator that allows freeing allocations in reverse order,
    rather than having to free the entire block at once. This is essentially accomplished by adding
    getter and setter functions for the offset.

    Allocation is performed in amortized O(1) time.
*/

#pragma once

#include <cstdlib>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cassert>

#define DEFAULT_ALIGNMENT alignof(max_align_t)

class StackAllocator
{
    private:
        size_t m_offset;
        size_t m_capacity;
        unsigned char *m_buffer;

    public:
        StackAllocator(size_t capacity);
        ~StackAllocator();
        void *alloc(size_t size, size_t alignment = DEFAULT_ALIGNMENT);
        size_t get_marker();
        void free_to_marker(size_t offset);
        void resize(size_t capacity);
        void free_all();
};

StackAllocator::StackAllocator(size_t capacity)
{
    m_offset = 0;
    m_capacity = capacity;
    m_buffer = static_cast<unsigned char*>(malloc(capacity));
}

StackAllocator::~StackAllocator()
{
    std::free(m_buffer);
}

void *StackAllocator::alloc(size_t size, size_t alignment)
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

size_t StackAllocator::get_marker()
{
    return m_offset;
}

void StackAllocator::free_to_marker(size_t offset)
{
    assert(offset >= 0 && offset < m_capacity);
    m_offset = offset;
}

void StackAllocator::resize(size_t capacity)
{
    if (capacity <= m_capacity)
        return;

    unsigned char *new_buffer = static_cast<unsigned char *>(malloc(capacity));
    memcpy(new_buffer, m_buffer, m_offset);
    std::free(m_buffer);
    m_buffer = new_buffer;
    m_capacity = capacity;
}

void StackAllocator::free_all()
{
    m_offset = 0;
}
