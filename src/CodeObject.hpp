#pragma once

#include "code_object_swap.hpp"
#include <cstddef>
#include <optional>

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

    static std::optional<CodeObject> try_read_from_file(const char* path);
};

} // namespace agent
