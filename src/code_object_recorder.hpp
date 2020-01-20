#pragma once

#include "code_object.hpp"
#include "logger/logger.hpp"
#include <shared_mutex>
#include <forward_list>
#include <functional>

namespace agent
{
class CodeObjectRecorder
{
private:
    call_count_t _load_call_counter;
    // Note: using a linked list instead of a vector because we pass references to items
    // and we don't want them to be invalidated on insertion.
    std::forward_list<RecordedCodeObject> _code_objects;
    std::string _dump_dir;
    std::shared_ptr<CodeObjectLogger> _logger;
    std::shared_mutex _mutex;

    std::string co_dump_path(crc32_t co_crc) const;
    void handle_crc_collision(const CodeObject& code_object);
    void dump_code_object(const CodeObject& code_object);
    void iterate_symbols(hsa_executable_t exec, RecordedCodeObject& code_object);

public:
    CodeObjectRecorder(std::string dump_dir, std::shared_ptr<CodeObjectLogger> logger)
        : _load_call_counter{0}, _dump_dir{dump_dir}, _logger{logger} {}
    RecordedCodeObject& record_code_object(const void* ptr, size_t size);
    std::optional<std::reference_wrapper<RecordedCodeObject>> record_code_object_executable(hsa_executable_t exec, hsa_code_object_reader_t reader);
    std::optional<std::reference_wrapper<RecordedCodeObject>> record_code_object_executable(hsa_executable_t exec, hsa_code_object_t hsaco);
};
} // namespace agent
