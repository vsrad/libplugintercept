#include "CodeObject.hpp"

#define CRCPP_USE_CPP11
#include "CRC.h"

CRC::Table<crc32_t, 32> _crc_table(CRC::CRC_32());

namespace agent
{
CodeObject::CodeObject(const void* ptr, size_t size)
    : _ptr{ptr},
      _size{size},
      _crc{CRC::Calculate(_ptr, size, _crc_table)} {}
} // namespace agent
