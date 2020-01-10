#include "CodeObjectManager.hpp"
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>

using namespace agent;

hsa_status_t iterate_symbols_callback(
    hsa_executable_t exec,
    hsa_executable_symbol_t symbol,
    void* data)
{
    auto co = reinterpret_cast<CodeObject*>(data);

    uint32_t name_len;
    hsa_status_t status = hsa_executable_symbol_get_info(symbol, HSA_EXECUTABLE_SYMBOL_INFO_NAME_LENGTH, &name_len);
    if (status != HSA_STATUS_SUCCESS)
        return status;

    char* name = new char[name_len];
    status = hsa_executable_symbol_get_info(symbol, HSA_EXECUTABLE_SYMBOL_INFO_NAME, name);
    if (status == HSA_STATUS_SUCCESS)
        co->add_symbol(std::string(name, name_len));

    delete[] name;
    return status;
}

std::string CodeObjectManager::co_dump_path(crc32_t co_crc) const
{
    return std::string(_dump_dir).append("/").append(std::to_string(co_crc)).append(".co");
}

void CodeObjectManager::handle_crc_collision(const CodeObject& code_object)
{
    auto filepath = co_dump_path(code_object.CRC());
    std::ifstream in(filepath, std::ios::binary | std::ios::ate);
    if (!in)
    {
        _logger->error(code_object, "cannot open " + filepath + " to check against a new code object with the same CRC");
        return;
    }

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
            _logger->error(code_object, "CRC collision: " + filepath);
        else
            _logger->warning(code_object, "redundant load: " + filepath);

        std::free(prev_ptr);
    }
    else
    {
        std::ostringstream msg;
        msg << "CRC collision: " << filepath << " (" << prev_size << " bytes) "
            << " vs new code object (" << code_object.Size() << " bytes)";
        _logger->error(code_object, msg.str());
    }
}

std::shared_ptr<CodeObject> CodeObjectManager::record_code_object(const void* ptr, size_t size)
{
    auto code_object = std::make_shared<CodeObject>(ptr, size);
    auto key = code_object->CRC();

    _logger->info(*code_object, "intercepted code object");

    std::unique_lock lock(_mutex);
    if (_code_objects.find(key) != _code_objects.end())
        handle_crc_collision(*code_object);
    else
        dump_code_object(*code_object);

    _code_objects[key] = code_object;
    return code_object;
}

void CodeObjectManager::dump_code_object(const CodeObject& code_object)
{
    auto filepath = co_dump_path(code_object.CRC());
    std::ofstream fs(filepath, std::ios::out | std::ios::binary);
    if (fs)
    {
        fs.write((char*)code_object.Ptr(), code_object.Size());
        fs.close();
        _logger->info(code_object, "code object is written to the file " + filepath);
    }
    else
    {
        _logger->error(code_object, "cannot write code object to the file " + filepath);
    }
}

void CodeObjectManager::set_code_object_handle(std::shared_ptr<CodeObject> co, hsa_code_object_reader_t reader)
{
    assert(reader.handle != 0);
    _code_objects_by_reader_handle[reader.handle] = co;
}

void CodeObjectManager::set_code_object_handle(std::shared_ptr<CodeObject> co, hsa_code_object_t hsaco)
{
    assert(hsaco.handle != 0);
    _code_objects_by_hsaco_handle[hsaco.handle] = co;
}

std::shared_ptr<CodeObject> CodeObjectManager::find_by_reader(hsa_code_object_reader_t reader)
{
    std::shared_lock lock(_mutex);
    auto it = _code_objects_by_reader_handle.find(reader.handle);
    if (it != _code_objects_by_reader_handle.end())
        return it->second;
    return {};
}

std::shared_ptr<CodeObject> CodeObjectManager::find_by_hsaco(hsa_code_object_t hsaco)
{
    std::shared_lock lock(_mutex);
    auto it = _code_objects_by_hsaco_handle.find(hsaco.handle);
    if (it != _code_objects_by_hsaco_handle.end())
        return it->second;
    return {};
}

void CodeObjectManager::iterate_symbols(hsa_executable_t exec, CodeObject& code_object)
{
    hsa_status_t status = hsa_executable_iterate_symbols(exec, iterate_symbols_callback, &code_object);
    if (status != HSA_STATUS_SUCCESS)
    {
        _logger->error(code_object, "cannot iterate symbols of executable: " + std::to_string(exec.handle));
        return;
    }

    std::ostringstream symbols_info_stream;
    symbols_info_stream << "found symbols:";

    for (const auto& symbol : code_object.get_symbols())
        symbols_info_stream << std::endl
                            << "-- " << symbol;

    const std::string& symbol_info_string = symbols_info_stream.str();
    _logger->info(code_object, symbol_info_string);
}

void CodeObjectManager::set_code_object_executable(hsa_executable_t exec, hsa_code_object_reader_t reader)
{
    if (auto co = find_by_reader(reader))
    {
        _code_objects_by_exec_handle[exec.handle] = co;
        iterate_symbols(exec, *co);
    }
    else
    {
        _logger->error("cannot find code object by hsa_code_object_reader_t: " + std::to_string(reader.handle));
    }
}

void CodeObjectManager::set_code_object_executable(hsa_executable_t exec, hsa_code_object_t hsaco)
{
    if (auto co = find_by_hsaco(hsaco))
    {
        _code_objects_by_exec_handle[exec.handle] = co;
        iterate_symbols(exec, *co);
    }
    else
    {
        _logger->error("cannot find code object by hsa_code_object_t: " + std::to_string(hsaco.handle));
    }
}
