#include "CodeObject.hpp"

#define CRCPP_USE_CPP11
#include "CRC.h"

#include <fstream>

CRC::Table<crc32_t, 32> _crc_table(CRC::CRC_32());

using namespace agent;

CodeObject::CodeObject(const void* ptr, size_t size)
    : _ptr{ptr},
      _size{size},
      _crc{CRC::Calculate(_ptr, size, _crc_table)} {}

std::optional<CodeObject> CodeObject::try_read_from_file(const char* path)
{
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in)
        return {};

    size_t size = in.tellg();
    char* co = new char[size];
    in.seekg(0, std::ios::beg);
    std::copy(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>(), co);

    return {CodeObject(co, size)};
}
