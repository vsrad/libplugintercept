#pragma once

#include <stddef.h>
#include <stdint.h>

typedef uint32_t crc32_t;

namespace agent
{

class CodeObject
{
private:
    const void* _ptr;
    const size_t _size;
    const crc32_t _crc;

public:
    CodeObject(const void* ptr, size_t size);
    const void* Ptr() const { return _ptr; }
    size_t Size() const { return _size; }
    crc32_t CRC() const { return _crc; }
};

} // namespace agent
