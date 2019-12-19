#pragma once

#include "CodeObject.hpp"
#include "logger/CodeObjectLogger.hpp"
#include <shared_mutex>
#include <sstream>
#include <memory>
#include <map>

namespace agent
{

class CodeObjectManager
{
private:
    std::map<uint32_t, std::shared_ptr<CodeObject>> _code_objects;
    std::shared_mutex _mutex;
    const std::string& _path;
    std::ostringstream _path_builder;
    agent::logger::CodeObjectLogger _logger;

    std::string CreateFilepath(std::string& filename);
    void CheckIdentitiyExistingCodeObject(agent::CodeObject& code_object);

public:
    CodeObjectManager(const std::string &path) : _path(path) {}
    std::shared_ptr<CodeObject> InitCodeObject(const void* ptr, size_t size);
    void WriteCodeObject(std::shared_ptr<CodeObject>& code_object);
};

} // namespace agent
