#pragma once

#include "CodeObject.hpp"
#include "logger/logger.hpp"
#include <memory>
#include <shared_mutex>
#include <sstream>
#include <unordered_map>

namespace agent
{

class CodeObjectManager
{
private:
    std::vector<std::shared_ptr<CodeObject>> _code_objects;
    std::unordered_map<decltype(hsa_executable_symbol_t::handle), std::pair<std::shared_ptr<CodeObject>, const std::string&>> _code_object_symbols;
    std::string _dump_dir;
    std::shared_ptr<CodeObjectLogger> _logger;
    std::shared_mutex _mutex;

    std::string co_dump_path(crc32_t co_crc) const;
    void handle_crc_collision(const CodeObject& code_object);
    void dump_code_object(const CodeObject& code_object);
    void iterate_symbols(hsa_executable_t exec, std::shared_ptr<CodeObject> code_object);

public:
    CodeObjectManager(std::string dump_dir, std::shared_ptr<CodeObjectLogger> logger)
        : _dump_dir{dump_dir}, _logger{logger} {}
    std::shared_ptr<CodeObject> record_code_object(const void* ptr, size_t size);
    std::shared_ptr<CodeObject> iterate_symbols(hsa_executable_t exec, hsa_code_object_reader_t reader);
    std::shared_ptr<CodeObject> iterate_symbols(hsa_executable_t exec, hsa_code_object_t hsaco);
    std::optional<std::pair<std::shared_ptr<CodeObject>, const std::string&>> lookup_symbol(hsa_executable_symbol_t symbol);
};

} // namespace agent
