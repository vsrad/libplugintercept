#include "CodeObjectManager.hpp"
#include <fstream>

namespace agent
{
CodeObjectManager::CodeObjectManager(std::string& path)
    : _path{std::move(path)} {}

std::shared_ptr<CodeObject> CodeObjectManager::InitCodeObject(const void* ptr, size_t size)
{
    auto code_object = std::shared_ptr<CodeObject>(new CodeObject(ptr, size));
    auto key = code_object->CRC();

    // TODO check if already exist code object

    _code_objects[key] = code_object;
    return code_object;
}

void CodeObjectManager::WriteCodeObject(std::shared_ptr<CodeObject>& code_object)
{
    _path_builder.clear();
    auto filename = std::to_string(code_object->CRC());

    _path_builder << _path << "/" << filename << ".co";
    auto filepath = _path_builder.str();

    std::ofstream fs(filepath, std::ios::out | std::ios::binary);
    if (!fs.is_open())
    {
        return;
    }

    fs.write((char*)code_object->Ptr(), code_object->Size());
    fs.close();
}
} // namespace agent