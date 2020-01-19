#include "code_object.hpp"

#define CRCPP_USE_CPP11
#include "CRC.h"

#include <fstream>

CRC::Table<crc32_t, 32> _crc_table(CRC::CRC_32());

using namespace agent;

CodeObject::CodeObject(const void* ptr, size_t size)
    : _ptr{ptr},
      _size{size},
      _crc{CRC::Calculate(_ptr, size, _crc_table)} {}

hsa_status_t RecordedCodeObject::fill_symbols(hsa_executable_t exec)
{
    return hsa_executable_iterate_symbols(exec, fill_symbols_callback, this);
}

hsa_status_t RecordedCodeObject::fill_symbols_callback(hsa_executable_t exec, hsa_executable_symbol_t sym, void* data)
{
    uint32_t name_len;
    hsa_status_t status = hsa_executable_symbol_get_info(sym, HSA_EXECUTABLE_SYMBOL_INFO_NAME_LENGTH, &name_len);
    if (status != HSA_STATUS_SUCCESS)
        return status;

    std::string name(name_len, '\0');
    status = hsa_executable_symbol_get_info(sym, HSA_EXECUTABLE_SYMBOL_INFO_NAME, name.data());
    if (status == HSA_STATUS_SUCCESS)
        reinterpret_cast<RecordedCodeObject*>(data)->_symbols.emplace(std::move(name), sym);

    return status;
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
