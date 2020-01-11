#pragma once

#include "code_object_swap.hpp"
#include <cstddef>
#include <hsa.h>
#include <map>
#include <optional>

namespace agent
{

class CodeObject
{
private:
    const void* _ptr;
    const size_t _size;
    const crc32_t _crc;
    std::map<decltype(hsa_executable_symbol_t::handle), std::string> _symbols;
    hsa_code_object_reader_t _reader = {0};
    hsa_code_object_t _hsaco = {0};

public:
    CodeObject(const void* ptr, size_t size);
    const void* Ptr() const { return _ptr; }
    size_t Size() const { return _size; }
    crc32_t CRC() const { return _crc; }
    const std::map<decltype(hsa_executable_symbol_t::handle), std::string>& symbols() const { return _symbols; }
    hsa_code_object_reader_t hsa_code_object_reader() const { return _reader; }
    hsa_code_object_t hsa_code_object() const { return _hsaco; }

    void add_symbol(hsa_executable_symbol_t sym, std::string name) { _symbols[sym.handle] = name; }
    void set_hsa_code_object_reader(hsa_code_object_reader_t reader) { _reader = reader; }
    void set_hsa_code_object(hsa_code_object_t hsaco) { _hsaco = hsaco; }

    static std::optional<CodeObject> try_read_from_file(const char* path);
};

} // namespace agent
