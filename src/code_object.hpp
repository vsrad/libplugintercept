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

public:
    CodeObject(const void* ptr, size_t size);
    const void* ptr() const { return _ptr; }
    size_t size() const { return _size; }
    crc32_t crc() const { return _crc; }

    static std::optional<CodeObject> try_read_from_file(const char* path);
};

class RecordedCodeObject : public CodeObject
{
private:
    std::map<std::string, hsa_executable_symbol_t> _symbols;
    hsa_code_object_reader_t _reader = {0};
    hsa_code_object_t _hsaco = {0};

    static hsa_status_t fill_symbols_callback(hsa_executable_t exec, hsa_executable_symbol_t sym, void* data);

public:
    RecordedCodeObject(const void* ptr, size_t size) : CodeObject(ptr, size) {}

    hsa_status_t fill_symbols(hsa_executable_t exec);
    const std::map<std::string, hsa_executable_symbol_t>& symbols() const { return _symbols; }

    void set_hsa_code_object_reader(hsa_code_object_reader_t reader) { _reader = reader; }
    hsa_code_object_reader_t hsa_code_object_reader() const { return _reader; }

    void set_hsa_code_object(hsa_code_object_t hsaco) { _hsaco = hsaco; }
    hsa_code_object_t hsa_code_object() const { return _hsaco; }
};

} // namespace agent
