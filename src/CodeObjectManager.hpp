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
    std::map<hsa_code_object_reader_t*, std::shared_ptr<CodeObject>> _code_objects_by_reader;
    std::map<hsa_code_object_t*, std::shared_ptr<CodeObject>> _code_objects_by_hsa_code_object;
    std::shared_mutex _mutex;
    std::string _path;
    std::ostringstream _path_builder;

    std::string CreateFilepath(std::string& filename);
    void CheckIdentitiyExistingCodeObject(agent::CodeObject& code_object);
    std::shared_ptr<CodeObject> InitCodeObject(const void* ptr, size_t size);

public:
    CodeObjectManager(std::string dump_dir, std::shared_ptr<CodeObjectLoggerInterface> logger)
        : _path{dump_dir}, _logger{logger} {}
    std::shared_ptr<CodeObjectLoggerInterface> _logger;
    std::shared_ptr<CodeObject> InitCodeObject(const void* ptr, size_t size, hsa_code_object_reader_t* co_reader);
    std::shared_ptr<CodeObject> InitCodeObject(const void* ptr, size_t size, hsa_code_object_t* code_object);
    void WriteCodeObject(std::shared_ptr<CodeObject>& code_object);
    std::shared_ptr<CodeObject> find_by_co_reader(hsa_code_object_reader_t& co_reader);
    std::shared_ptr<CodeObject> find_by_hsa_code_object(hsa_code_object_t& code_object);
    void iterate_symbols(hsa_executable_t& exec, std::shared_ptr<CodeObject>& code_object);
    std::shared_ptr<CodeObject> iterate_symbols(hsa_executable_t& exec, hsa_code_object_reader_t& reader);
    std::shared_ptr<CodeObject> iterate_symbols(hsa_executable_t& exec, hsa_code_object_t& code_object);
};

} // namespace agent
