#pragma once

#include <stddef.h>
#include <stdint.h>

namespace agent
{

class CodeObject
{
private:
    const void* _ptr;
    size_t _size;
    uint32_t _crc;
    uint32_t GenerateCRC();

public:
    CodeObject(const void* ptr, size_t size);
    const void* Ptr();
    size_t Size();
    uint32_t CRC();
};

} // namespace agent