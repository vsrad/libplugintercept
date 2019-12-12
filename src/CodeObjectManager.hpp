#pragma once

#include "CodeObject.hpp"
#include <string>
#include <map>

namespace agent
{

class CodeObjectManager
{
private:
    std::map<uint32_t, CodeObject* > _code_objects;
    std::string _path;

public:
    CodeObjectManager(std::string &path);
    CodeObject& InitCodeObject(const void* ptr, size_t size);
    void WriteCodeObject(CodeObject &code_object);
};

} // namespace agent