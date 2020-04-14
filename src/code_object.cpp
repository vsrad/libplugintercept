#include "code_object.hpp"

#define CRCPP_USE_CPP11
#include "CRC.h"

#include <fstream>
#include <iomanip>
#include <sstream>

CRC::Table<crc32_t, 32> _crc_table(CRC::CRC_32());

using namespace agent;

CodeObject::CodeObject(const void* ptr, size_t size)
    : _ptr{ptr},
      _size{size},
      _crc{CRC::Calculate(_ptr, size, _crc_table)} {}

bool RecordedCodeObject::hsaco_eq(const hsaco_t* other) const
{
    if (auto reader_lhs = std::get_if<hsa_code_object_reader_t>(&_hsaco))
    {
        if (auto reader_rhs = std::get_if<hsa_code_object_reader_t>(other))
            return reader_lhs->handle == reader_rhs->handle;
    }
    else if (auto cobj_lhs = std::get_if<hsa_code_object_t>(&_hsaco))
    {
        if (auto cobj_rhs = std::get_if<hsa_code_object_t>(other))
            return cobj_lhs->handle == cobj_rhs->handle;
    }
    return false;
}

std::string RecordedCodeObject::info() const
{
    std::stringstream info;
    info << "0x" << std::setfill('0') << std::setw(sizeof(crc32_t) * 2) << std::hex << crc();
    info << " (co-load-id " << std::setw(0) << std::dec << load_call_id() << ")";
    return info.str();
}

std::string RecordedCodeObject::dump_path(const std::string& dump_dir) const
{
    std::stringstream path;
    path << dump_dir << "/";
    path << std::setfill('0') << std::setw(sizeof(crc32_t) * 2) << std::hex << crc();
    path << ".co";
    return path.str();
}

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
