#include "CodeObject.hpp"
#include "CRC.h"

CRC::Table<uint32_t, 32> _crc_table(CRC::CRC_32());

namespace agent
{
CodeObject::CodeObject(const void* ptr, size_t size)
    : _ptr{ptr},
      _size{size},
      _crc{0} {}

template <typename T>
T* CodeObject::Ptr()
{
    return (T*)_ptr;
}

size_t CodeObject::Size()
{
    return _size;
}

uint32_t CodeObject::CRC()
{
    return (_crc) ? _crc : GenerateCRC();
}

uint32_t CodeObject::GenerateCRC()
{
    return CRC::Calculate(_ptr, _size, _crc_table);
}

} // namespace agent