#pragma once

#include <cstddef>
#include <hsa.h>
#include <map>
#include <optional>
#include <variant>

typedef uint32_t crc32_t;
typedef uint32_t load_call_id_t;
typedef uint32_t get_info_call_id_t;

typedef std::variant<hsa_code_object_t, hsa_code_object_reader_t> hsaco_t;

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
    const load_call_id_t _load_call_id;
    const hsaco_t _hsaco;
    std::map<decltype(hsa_executable_symbol_t::handle), std::string> _symbols;

    static hsa_status_t fill_symbols_callback(hsa_executable_t exec, hsa_executable_symbol_t sym, void* data);

public:
    RecordedCodeObject(const void* ptr, size_t size, load_call_id_t load_call_id, hsaco_t hsaco)
        : CodeObject(ptr, size), _load_call_id(load_call_id), _hsaco(hsaco) {}
    // Recorded code object are always stored in a single location (CodeObjectRecorder);
    // any copy would most likely be done in error.
    RecordedCodeObject(const RecordedCodeObject&) = delete;

    std::string info() const;
    std::string dump_path(const std::string& dump_dir) const;

    load_call_id_t load_call_id() const { return _load_call_id; }
    bool hsaco_eq(const hsaco_t* other) const;

    hsa_status_t fill_symbols(hsa_executable_t exec);
    const std::map<decltype(hsa_executable_symbol_t::handle), std::string>& symbols() const { return _symbols; }
};
} // namespace agent
