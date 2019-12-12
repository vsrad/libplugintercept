#include "CodeObjectManager.hpp"
#include <fstream>

namespace agent
{
CodeObjectManager::CodeObjectManager(std::string& path)
    : _path{std::move(path)} {}

CodeObject& CodeObjectManager::InitCodeObject(const void* ptr, size_t size)
{
    auto code_object = new CodeObject(ptr, size);
    auto key = code_object->CRC();

    // TODO check if already exist code object

    _code_objects[key] = code_object;
    return *code_object;
}

void CodeObjectManager::WriteCodeObject(CodeObject& code_object)
{
    auto filename = std::to_string(code_object.CRC());
    std::ofstream fs(_path + filename, std::ios::out | std::ios::binary);
    if (!fs.is_open())
    {
        return;
    }

    fs.write(code_object.Ptr<char>(), code_object.Size());
    fs.close();
}
} // namespace agent