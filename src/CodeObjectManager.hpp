#pragma once

#include "CodeObject.hpp"
#include "hsa.h"
#include "logger/logger.hpp"
#include <map>
#include <memory>
#include <shared_mutex>
#include <sstream>

namespace agent
{

class CodeObjectManager
{
private:
    std::map<uint32_t, std::shared_ptr<CodeObject>> _code_objects;
    std::map<decltype(hsa_code_object_reader_t::handle), std::shared_ptr<CodeObject>> _code_objects_by_reader_handle;
    std::map<decltype(hsa_code_object_t::handle), std::shared_ptr<CodeObject>> _code_objects_by_hsaco_handle;
    std::string _dump_dir;
    std::shared_ptr<CodeObjectLoggerInterface> _logger;
    std::shared_mutex _mutex;

    std::string co_dump_path(crc32_t co_crc) const;
    void handle_crc_collision(const CodeObject& code_object);
    void dump_code_object(const CodeObject& code_object);
    std::shared_ptr<CodeObject> find_by_reader(hsa_code_object_reader_t reader);
    std::shared_ptr<CodeObject> find_by_hsaco(hsa_code_object_t hsaco);
    void iterate_symbols(hsa_executable_t exec, CodeObject& code_object);

public:
    CodeObjectManager(std::string dump_dir, std::shared_ptr<CodeObjectLoggerInterface> logger)
        : _dump_dir{dump_dir}, _logger{logger} {}
    std::shared_ptr<CodeObject> record_code_object(const void* ptr, size_t size);
    void set_code_object_handle(std::shared_ptr<CodeObject> co, hsa_code_object_reader_t reader);
    void set_code_object_handle(std::shared_ptr<CodeObject> co, hsa_code_object_t hsaco);
    void iterate_symbols(hsa_executable_t exec, hsa_code_object_reader_t reader);
    void iterate_symbols(hsa_executable_t exec, hsa_code_object_t hsaco);
};

} // namespace agent
