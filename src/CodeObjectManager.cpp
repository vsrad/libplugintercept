#include "CodeObjectManager.hpp"
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>

using namespace agent;

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
    _logger->info(*code_object, "intercepted code object");

    auto crc_eq = [crc = code_object->CRC()](auto const& co) { return co->CRC() == crc; };

    std::unique_lock lock(_mutex);
    if (std::find_if(_code_objects.begin(), _code_objects.end(), crc_eq) != _code_objects.end())
        handle_crc_collision(*code_object);
    else
        dump_code_object(*code_object);

    _code_objects.push_back(code_object);
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

void CodeObjectManager::iterate_symbols(hsa_executable_t exec, std::shared_ptr<CodeObject> code_object)
{
    if (code_object->fill_symbols(exec) != HSA_STATUS_SUCCESS)
    {
        _logger->error(*code_object, "cannot iterate symbols of executable: " + std::to_string(exec.handle));
        return;
    }

    std::ostringstream symbols_info_stream;
    symbols_info_stream << "found symbols:";

    for (const auto& [sym_handle, name] : code_object->symbols())
    {
        _code_objects_by_symbol[sym_handle] = code_object;
        symbols_info_stream << std::endl
                            << "-- " << name;
    }

    _logger->info(*code_object, symbols_info_stream.str());
}

void CodeObjectManager::iterate_symbols(hsa_executable_t exec, hsa_code_object_reader_t reader)
{
    std::shared_lock lock(_mutex);
    auto reader_eq = [hndl = reader.handle](auto const& co) { return co->hsa_code_object_reader().handle == hndl; };
    auto it = std::find_if(_code_objects.begin(), _code_objects.end(), reader_eq);
    if (it != _code_objects.end())
        iterate_symbols(exec, *it);
    else
        _logger->error("cannot find code object by hsa_code_object_reader_t: " + std::to_string(reader.handle));
}

void CodeObjectManager::iterate_symbols(hsa_executable_t exec, hsa_code_object_t hsaco)
{
    std::shared_lock lock(_mutex);
    auto hsaco_eq = [hndl = hsaco.handle](auto const& co) { return co->hsa_code_object().handle == hndl; };
    auto it = std::find_if(_code_objects.begin(), _code_objects.end(), hsaco_eq);
    if (it != _code_objects.end())
        iterate_symbols(exec, *it);
    else
        _logger->error("cannot find code object by hsa_code_object_t: " + std::to_string(hsaco.handle));
}
