#include "CodeObjectManager.hpp"
#include <cstring>
#include <fstream>
#include <iostream>

hsa_status_t iterate_symbols_callback(
    hsa_executable_t exec,
    hsa_executable_symbol_t symbol,
    void* data)
{
    auto co = reinterpret_cast<agent::CodeObject*>(data);

    uint32_t name_len;
    hsa_status_t status = hsa_executable_symbol_get_info(symbol, HSA_EXECUTABLE_SYMBOL_INFO_NAME_LENGTH, &name_len);
    if (status != HSA_STATUS_SUCCESS)
        return status;

    char* name = new char[name_len];
    status = hsa_executable_symbol_get_info(symbol, HSA_EXECUTABLE_SYMBOL_INFO_NAME, name);
    if (status == HSA_STATUS_SUCCESS)
        co->add_symbol(std::string(name));

    delete[] name;
    return status;
}

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
                    _logger->error(code_object, "code object not equals with last code object: " + filepath);
                else
                    _logger->warning(code_object, "redundant load: " + filepath);

                std::free(prev_ptr);
            }
            else
            {
                _logger->error(code_object, "code object not equals with last code object: " + filepath);
            }
        }
        else
        {
            _logger->error(code_object, "cannot open code object file to check equivalence of new input code object: " + filepath);
        }
    }
}

std::shared_ptr<CodeObject> CodeObjectManager::InitCodeObject(const void* ptr, size_t size)
{
    auto code_object = std::shared_ptr<CodeObject>(new CodeObject(ptr, size));
    auto key = code_object->CRC();

    _logger->info(*code_object, "intercepted code object");

    std::unique_lock lock(_mutex);
    if (_code_objects.find(key) != _code_objects.end())
        CheckIdentitiyExistingCodeObject(*code_object);

    _code_objects[key] = code_object;
    return code_object;
}

std::shared_ptr<CodeObject> CodeObjectManager::InitCodeObject(const void* ptr, size_t size, hsa_code_object_reader_t* co_reader)
{
    auto co = InitCodeObject(ptr, size);

    _code_objects_by_reader[co_reader] = co;
    return co;
}

std::shared_ptr<CodeObject> CodeObjectManager::InitCodeObject(const void* ptr, size_t size, hsa_code_object_t* code_object)
{
    auto co = InitCodeObject(ptr, size);

    _code_objects_by_hsa_code_object[code_object] = co;
    return co;
}

void CodeObjectManager::WriteCodeObject(std::shared_ptr<CodeObject>& code_object)
{
    auto filename = std::to_string(code_object->CRC());
    auto filepath = CreateFilepath(filename);

    std::shared_lock lock(_mutex);
    std::ofstream fs(filepath, std::ios::out | std::ios::binary);
    if (!fs.is_open())
    {
        _logger->error(*code_object, "cannot write code object to the file " + filepath);
        return;
    }

    fs.write((char*)code_object->Ptr(), code_object->Size());
    fs.close();

    _logger->info(*code_object, "code object is written to the file " + filepath);
}

std::shared_ptr<CodeObject> CodeObjectManager::find_by_co_reader(hsa_code_object_reader_t& co_reader)
{
    std::shared_lock lock(_mutex);
    for (const auto& record : _code_objects_by_reader)
    {
        if (record.first != nullptr && record.first->handle == co_reader.handle)
            return record.second;
    }

    return nullptr;
}

std::shared_ptr<CodeObject> CodeObjectManager::find_by_hsa_code_object(hsa_code_object_t& code_object)
{
    std::shared_lock lock(_mutex);
    for (const auto& record : _code_objects_by_hsa_code_object)
    {
        if (record.first != nullptr && record.first->handle == code_object.handle)
            return record.second;
    }

    return nullptr;
}

void CodeObjectManager::iterate_symbols(hsa_executable_t& exec, std::shared_ptr<CodeObject>& code_object)
{
    hsa_status_t status = hsa_executable_iterate_symbols(exec, iterate_symbols_callback, code_object.get());
    if (status != HSA_STATUS_SUCCESS)
    {
        _logger->error(*code_object, "cannot iterate symbols of executable: " + exec.handle);
        return;
    }

    std::ostringstream symbols_info_stream;
    symbols_info_stream << "found symbols:";

    for (const auto& symbol : code_object->get_symbols())
        symbols_info_stream << std::endl
                            << "-- " << symbol;

    const std::string& symbol_info_string = symbols_info_stream.str();
    _logger->info(*code_object, symbol_info_string);
}

std::shared_ptr<CodeObject> CodeObjectManager::iterate_symbols(hsa_executable_t& exec, hsa_code_object_reader_t& co_reader)
{
    auto co = find_by_co_reader(co_reader);

    if (co)
        iterate_symbols(exec, co);
    else
        _logger->error("cannot find code object by hsa_code_object_reader_t: " + std::to_string(co_reader.handle));
    return co;
}

std::shared_ptr<CodeObject> CodeObjectManager::iterate_symbols(hsa_executable_t& exec, hsa_code_object_t& code_object)
{
    auto co = find_by_hsa_code_object(code_object);

    if (co)
        iterate_symbols(exec, co);
    else
        _logger->error("cannot find code object by hsa_code_object_t: " + std::to_string(code_object.handle));
    return co;
}
} // namespace agent
