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
    std::shared_mutex _mutex;
    std::string _path;
    std::ostringstream _path_builder;

    std::string CreateFilepath(std::string& filename);
    void CheckIdentitiyExistingCodeObject(agent::CodeObject& code_object);

    std::shared_ptr<CodeObject> find_by_reader(hsa_code_object_reader_t reader);
    std::shared_ptr<CodeObject> find_by_hsaco(hsa_code_object_t hsaco);
    void iterate_symbols(hsa_executable_t exec, CodeObject& code_object);

public:
    CodeObjectManager(std::string dump_dir, std::shared_ptr<CodeObjectLoggerInterface> logger)
        : _path{dump_dir}, _logger{logger} {}
    std::shared_ptr<CodeObjectLoggerInterface> _logger;
    std::shared_ptr<CodeObject> InitCodeObject(const void* ptr, size_t size);
    void WriteCodeObject(std::shared_ptr<CodeObject>& code_object);
    void set_code_object_handle(std::shared_ptr<CodeObject> co, hsa_code_object_reader_t reader);
    void set_code_object_handle(std::shared_ptr<CodeObject> co, hsa_code_object_t hsaco);
    void iterate_symbols(hsa_executable_t exec, hsa_code_object_reader_t reader);
    void iterate_symbols(hsa_executable_t exec, hsa_code_object_t hsaco);
};

} // namespace agent
