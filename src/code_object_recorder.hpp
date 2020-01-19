#pragma once

#include "code_object.hpp"
#include "logger/logger.hpp"
#include <memory>
#include <shared_mutex>
#include <sstream>
#include <unordered_map>

namespace agent
{

class CodeObjectRecorder
{
private:
    std::vector<std::shared_ptr<RecordedCodeObject>> _code_objects;
    std::string _dump_dir;
    std::shared_ptr<CodeObjectLogger> _logger;
    std::shared_mutex _mutex;

    std::string co_dump_path(crc32_t co_crc) const;
    void handle_crc_collision(const CodeObject& code_object);
    void dump_code_object(const CodeObject& code_object);
    void iterate_symbols(hsa_executable_t exec, std::shared_ptr<RecordedCodeObject> code_object);

public:
    CodeObjectRecorder(std::string dump_dir, std::shared_ptr<CodeObjectLogger> logger)
        : _dump_dir{dump_dir}, _logger{logger} {}
    std::shared_ptr<RecordedCodeObject> record_code_object(const void* ptr, size_t size);
    std::shared_ptr<RecordedCodeObject> iterate_symbols(hsa_executable_t exec, hsa_code_object_reader_t reader);
    std::shared_ptr<RecordedCodeObject> iterate_symbols(hsa_executable_t exec, hsa_code_object_t hsaco);
};

} // namespace agent
