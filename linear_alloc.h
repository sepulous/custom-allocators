/*
    The linear allocator (aka "bump allocator") is the simplest possible allocator.

    Allocation is performed in amortized O(1) time.

    Only one buffer is utilized, which does not grow dynamically, and must be manually resized by the user.

    The linear allocator is often conflated with the arena allocator, but the latter is actually a higher-level system
    which grows dynamically.
*/

#ifndef LINEAR_ALLOC_H
#define LINEAR_ALLOC_H

#include <cstdlib>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cassert>

class LinearAllocator
{
    private:
        size_t _offset;
        size_t _capacity;
        unsigned char *_buffer;

    public:
        LinearAllocator(size_t capacity);
        ~LinearAllocator();
        void *alloc(size_t size);
        void *alloc_align(size_t size, size_t alignment);
        void resize(size_t capacity);
        void free();
};

LinearAllocator::LinearAllocator(size_t capacity)
{
    _offset = 0;
    _capacity = capacity;
    _buffer = static_cast<unsigned char*>(malloc(capacity));
}

LinearAllocator::~LinearAllocator()
{
    std::free(_buffer);
}

void *LinearAllocator::alloc(size_t size)
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

void *LinearAllocator::alloc_align(size_t size, size_t alignment)
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

void LinearAllocator::resize(size_t capacity)
{
    if (capacity <= _capacity)
        return;

    unsigned char *new_buffer = static_cast<unsigned char *>(malloc(capacity));
    memcpy(new_buffer, _buffer, _offset);
    std::free(_buffer);
    _buffer = new_buffer;
    _capacity = capacity;
}

void LinearAllocator::free()
{
    _offset = 0;
}

#endif
