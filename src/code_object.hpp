#pragma once

#include <cstddef>
#include <hsa.h>
#include <map>
#include <optional>

typedef uint32_t crc32_t;
typedef uint64_t call_count_t;

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
    CodeObject(CodeObject&& other) : _ptr{other._ptr}, _size(other._size), _crc{other._crc} {}
    const void* ptr() const { return _ptr; }
    size_t size() const { return _size; }
    crc32_t crc() const { return _crc; }

    static std::optional<CodeObject> try_read_from_file(const char* path);
};

class RecordedCodeObject : public CodeObject
{
private:
    const call_count_t _load_call_no;
    std::map<std::string, hsa_executable_symbol_t> _symbols;
    hsa_code_object_reader_t _reader = {0};
    hsa_code_object_t _hsaco = {0};

    static hsa_status_t fill_symbols_callback(hsa_executable_t exec, hsa_executable_symbol_t sym, void* data);

public:
    RecordedCodeObject(const void* ptr, size_t size, call_count_t load_call_no)
        : CodeObject(ptr, size), _load_call_no(load_call_no) {}
    // Recorded code object are always stored in a single location (CodeObjectRecorder);
    // any copy would most likely be done in error.
    RecordedCodeObject(const RecordedCodeObject&) = delete;

    call_count_t load_call_no() const { return _load_call_no; }

    hsa_status_t fill_symbols(hsa_executable_t exec);
    const std::map<std::string, hsa_executable_symbol_t>& symbols() const { return _symbols; }

    void set_hsa_code_object_reader(hsa_code_object_reader_t reader) { _reader = reader; }
    hsa_code_object_reader_t hsa_code_object_reader() const { return _reader; }

    void set_hsa_code_object(hsa_code_object_t hsaco) { _hsaco = hsaco; }
    hsa_code_object_t hsa_code_object() const { return _hsaco; }
};
} // namespace agent
