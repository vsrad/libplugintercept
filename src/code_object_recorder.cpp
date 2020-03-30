#include "code_object_recorder.hpp"
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>

using namespace agent;

std::string CodeObjectRecorder::co_dump_path(crc32_t co_crc) const
{
    return std::string(_dump_dir).append("/").append(std::to_string(co_crc)).append(".co");
}

void CodeObjectRecorder::handle_crc_collision(const CodeObject& code_object)
{
    auto filepath = co_dump_path(code_object.crc());
    std::ifstream in(filepath, std::ios::binary | std::ios::ate);
    if (!in)
    {
        _logger->error(code_object, "cannot open " + filepath + " to check against a new code object with the same CRC");
        return;
    }

    size_t prev_size = std::string::size_type(in.tellg());
    if (prev_size == code_object.size())
    {
        char* prev_ptr = (char*)std::malloc(code_object.size());
        in.seekg(0, std::ios::beg);
        std::copy(std::istreambuf_iterator<char>(in),
                  std::istreambuf_iterator<char>(),
                  prev_ptr);

        auto res = std::memcmp(code_object.ptr(), prev_ptr, code_object.size());
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
            << " vs new code object (" << code_object.size() << " bytes)";
        _logger->error(code_object, msg.str());
    }
}

RecordedCodeObject& CodeObjectRecorder::record_code_object(const void* ptr, size_t size)
{
    std::scoped_lock lock(_mutex);
    auto load_call_id = ++_load_call_counter;

    _code_objects.emplace_front(ptr, size, load_call_id);
    auto& code_object = _code_objects.front();

    _logger->info(code_object, "intercepted code object");

    auto crc_eq = [load_call_id, crc = code_object.crc()](auto const& co) { return co.load_call_id() != load_call_id && co.crc() == crc; };
    if (std::find_if(_code_objects.begin(), _code_objects.end(), crc_eq) != _code_objects.end())
        handle_crc_collision(code_object);
    else
        dump_code_object(code_object);

    return code_object;
}

void CodeObjectRecorder::dump_code_object(const CodeObject& code_object)
{
    auto filepath = co_dump_path(code_object.crc());
    if (std::ofstream fs{filepath, std::ios::out | std::ios::binary})
    {
        fs.write((char*)code_object.ptr(), code_object.size());
        _logger->info(code_object, "code object is written to " + filepath);
    }
    else
    {
        _logger->error(code_object, "cannot write code object to " + filepath);
    }
}

void CodeObjectRecorder::iterate_symbols(hsa_executable_t exec, RecordedCodeObject& code_object)
{
    if (code_object.fill_symbols(exec) != HSA_STATUS_SUCCESS)
    {
        _logger->error(code_object, "cannot iterate symbols of executable: " + std::to_string(exec.handle));
    }
    else if (code_object.symbols().empty())
    {
        _logger->info(code_object, "no symbols found");
    }
    else
    {
        std::string symbols;
        for (const auto& it : code_object.symbols())
            symbols.append(symbols.empty() ? "code object symbols: " : ", ").append(it.first);
        _logger->info(code_object, symbols);
    }
}

std::optional<std::reference_wrapper<RecordedCodeObject>> CodeObjectRecorder::find_code_object(hsa_code_object_reader_t reader)
{
    std::shared_lock lock(_mutex);
    auto reader_eq = [hndl = reader.handle](auto const& co) { return co.hsa_code_object_reader().handle == hndl; };
    if (auto it{std::find_if(_code_objects.begin(), _code_objects.end(), reader_eq)}; it != _code_objects.end())
        return {std::ref(*it)};

    _logger->error("cannot find code object by hsa_code_object_reader_t: " + std::to_string(reader.handle));
    return {};
}

std::optional<std::reference_wrapper<RecordedCodeObject>> CodeObjectRecorder::find_code_object(hsa_code_object_t hsaco)
{
    std::shared_lock lock(_mutex);
    auto hsaco_eq = [hndl = hsaco.handle](auto const& co) { return co.hsa_code_object().handle == hndl; };
    if (auto it{std::find_if(_code_objects.begin(), _code_objects.end(), hsaco_eq)}; it != _code_objects.end())
        return {std::ref(*it)};

    _logger->error("cannot find code object by hsa_code_object_t: " + std::to_string(hsaco.handle));
    return {};
}
