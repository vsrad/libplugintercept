#include "CodeObjectManager.hpp"
#include <cstring>
#include <fstream>
#include <iostream>

namespace agent
{
std::string CodeObjectManager::CreateFilepath(std::string& filename)
{
    _path_builder.str("");
    _path_builder.clear();
    _path_builder << _path << "/" << filename << ".co";

    return _path_builder.str();
}

void CodeObjectManager::CheckIdentitiyExistingCodeObject(agent::CodeObject& code_object)
{
    auto filename = std::to_string(code_object.CRC());
    auto filepath = CreateFilepath(filename);

    std::ifstream in(filepath, std::ios::binary | std::ios::ate);
    if (!in.fail())
    {
        if (in.is_open())
        {
            size_t prev_size = std::string::size_type(in.tellg());

            if (prev_size == code_object.Size())
            {
                char* prev_ptr = (char*)std::malloc(code_object.Size());
                in.seekg(0, std::ios::beg);
                std::copy(std::istreambuf_iterator<char>(in),
                          std::istreambuf_iterator<char>(),
                          prev_ptr);

                auto res = std::memcmp(code_object.Ptr(), prev_ptr, code_object.Size());
                if (res)
                    _logger.Log(code_object, agent::logger::ERROR, "code object not equals with preview code object");
                else
                    _logger.Log(code_object, agent::logger::WARNING, "redundant load");

                std::free(prev_ptr);
            }
            else
            {
                _logger.Log(code_object, agent::logger::ERROR, "code object not equals with preview code object");
            }
        }
        else
        {
            _logger.Log(code_object, agent::logger::ERROR, "cannot open code object file to check equivalence of new input code object");
        }
    }
}

std::shared_ptr<CodeObject> CodeObjectManager::InitCodeObject(const void* ptr, size_t size, hsa_code_object_reader_t* co_reader)
{
    auto code_object = std::shared_ptr<CodeObject>(new CodeObject(ptr, size));
    auto key = code_object->CRC();

    _logger.Log(*code_object, agent::logger::INFO, "intercepted code object");

    std::unique_lock lock(_mutex);
    if (_code_objects.find(key) != _code_objects.end())
        CheckIdentitiyExistingCodeObject(*code_object);

    _code_objects[key] = code_object;
    _code_objects_by_reader[co_reader] = code_object;
    return code_object;
}

void CodeObjectManager::WriteCodeObject(std::shared_ptr<CodeObject>& code_object)
{
    auto filename = std::to_string(code_object->CRC());
    auto filepath = CreateFilepath(filename);

    std::shared_lock lock(_mutex);
    std::ofstream fs(filepath, std::ios::out | std::ios::binary);
    if (!fs.is_open())
    {
        _logger.Log(*code_object, agent::logger::ERROR, "cannot write code object to the file");
        return;
    }

    fs.write((char*)code_object->Ptr(), code_object->Size());
    fs.close();

    _logger.Log(*code_object, agent::logger::INFO, "code object is written to file");
}

std::shared_ptr<CodeObject> CodeObjectManager::find_by_co_reader(hsa_code_object_reader_t &co_reader)
{
    std::shared_lock lock(_mutex);
    for (const auto& record : _code_objects_by_reader)
    {
        if (record.first != nullptr && record.first->handle == co_reader.handle)
            return record.second;
    }

    return nullptr;
}
} // namespace agent
