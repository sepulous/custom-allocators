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

class StackAllocator
{
    private:
        size_t _offset;
        size_t _capacity;
        unsigned char *_buffer;

    public:
        StackAllocator(size_t capacity);
        ~StackAllocator();
        void *alloc(size_t size);
        void *alloc_align(size_t size, size_t alignment);
        size_t get_marker();
        void free_to_marker(size_t offset);
        void resize(size_t capacity);
        void free_all();
};

StackAllocator::StackAllocator(size_t capacity)
{
    _offset = 0;
    _capacity = capacity;
    _buffer = static_cast<unsigned char*>(malloc(capacity));
}

StackAllocator::~StackAllocator()
{
    std::free(_buffer);
}

void *StackAllocator::alloc(size_t size)
{
    constexpr size_t DEFAULT_ALIGNMENT = alignof(max_align_t);

    size_t corrected_offset = (_offset + DEFAULT_ALIGNMENT - 1) & ~(DEFAULT_ALIGNMENT - 1);
    if (size <= _capacity - corrected_offset)
    {
        _offset = corrected_offset + size;
        return &_buffer[corrected_offset];
    }
    return nullptr; // Out of space
}

void *StackAllocator::alloc_align(size_t size, size_t alignment)
{
    assert((alignment & (alignment - 1)) == 0); // Alignment must be a power of two

    size_t corrected_offset = (_offset + alignment - 1) & ~(alignment - 1);
    if (size <= _capacity - corrected_offset)
    {
        _offset = corrected_offset + size;
        return &_buffer[corrected_offset];
    }
    return nullptr; // Out of space
}

size_t StackAllocator::get_marker()
{
    return _offset;
}

void StackAllocator::free_to_marker(size_t offset)
{
    assert(offset >= 0 && offset < _capacity);
    _offset = offset;
}

void StackAllocator::resize(size_t capacity)
{
    if (capacity <= _capacity)
        return;

    unsigned char *new_buffer = static_cast<unsigned char *>(malloc(capacity));
    memcpy(new_buffer, _buffer, _offset);
    std::free(_buffer);
    _buffer = new_buffer;
    _capacity = capacity;
}

void StackAllocator::free_all()
{
    _offset = 0;
}
