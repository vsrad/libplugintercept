#pragma once

#include "CodeObject.hpp"
#include <sstream>
#include <memory>
#include <map>

namespace agent
{

class CodeObjectManager
{
private:
    std::map<uint32_t, std::shared_ptr<CodeObject>> _code_objects;
    std::string _path;
    std::ostringstream _path_builder;

public:
    CodeObjectManager(std::string &path);
    std::shared_ptr<CodeObject> InitCodeObject(const void* ptr, size_t size);
    void WriteCodeObject(std::shared_ptr<CodeObject>& code_object);
};

} // namespace agent