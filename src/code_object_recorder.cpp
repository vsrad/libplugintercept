#include "code_object_recorder.hpp"
#include <cassert>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>

using namespace agent;

void CodeObjectRecorder::record_code_object(const void* ptr, size_t size, hsaco_t hsaco, hsa_status_t load_status)
{
    std::scoped_lock lock(_mutex);
    auto load_call_id = ++_load_call_counter;
    if (load_status != HSA_STATUS_SUCCESS)
    {
        _logger->warning("Load #" + std::to_string(load_call_id) + " failed with status " + std::to_string(load_status));
        return;
    }

    auto& code_object = _code_objects.emplace_front(ptr, size, load_call_id, hsaco);
    _logger->info(code_object, "loaded");

    auto crc_eq = [load_call_id, crc = code_object.crc()](const auto& co) { return co.load_call_id() != load_call_id && co.crc() == crc; };
    auto collision = std::find_if(_code_objects.begin(), _code_objects.end(), crc_eq);
    if (collision != _code_objects.end())
        handle_crc_collision(code_object, *collision);
    else
        dump_code_object(code_object);
}

void CodeObjectRecorder::dump_code_object(const RecordedCodeObject& co)
{
    if (_dump_dir.empty())
        return;

    auto filepath = co.dump_path(_dump_dir);
    if (std::ofstream fs{filepath, std::ios::out | std::ios::binary})
    {
        fs.write((char*)co.ptr(), co.size());
        _logger->info(co, "written to " + filepath);
    }
    else
    {
        _logger->error(co, "cannot write code object to " + filepath);
    }
}

void CodeObjectRecorder::handle_crc_collision(const RecordedCodeObject& new_co, const RecordedCodeObject& existing_co)
{
    if (new_co.size() != existing_co.size())
    {
        _logger->error(new_co, "CRC collision with CO " + existing_co.info() +
                                   " (" + std::to_string(new_co.size()) + " bytes new, " + std::to_string(existing_co.size()) + " bytes existing)");
        return;
    }
    // We have a pointer to the contents of the existing code object,
    // but we cannot directly read it because the memory area could've been already freed.
    if (_dump_dir.empty())
    {
        _logger->warning("has the same size as CO " + existing_co.info() + ". " +
                         "This is most likely a redundant load. Specify code object dump directory to compare their contents to check for a CRC collision.");
        return;
    }
    // If we dump code objects to disk, we can load the contents of the existing code object
    // and compare them to the new code object.
    auto filepath = existing_co.dump_path(_dump_dir);
    std::ifstream in(filepath, std::ios::binary);
    if (!in)
    {
        _logger->error(new_co, "has the same size as CO " + existing_co.info() + ", but their contents could not be compared: could not read " + filepath);
        return;
    }
    char* existing_co_contents = (char*)std::malloc(existing_co.size());
    std::copy(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>(), existing_co_contents);
    if (std::memcmp(new_co.ptr(), existing_co_contents, existing_co.size()))
        _logger->error(new_co, "CRC collision with CO " + existing_co.info() + " (dumped to " + filepath + ")");
    else
        _logger->warning(new_co, "redundant load, same contents as CO " + existing_co.info());
    std::free(existing_co_contents);
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

std::optional<std::reference_wrapper<RecordedCodeObject>> CodeObjectRecorder::find_code_object(const hsaco_t* hsaco)
{
    std::shared_lock lock(_mutex);
    auto hsaco_eq = [hsaco](const auto& co) { return co.hsaco_eq(hsaco); };
    if (auto it{std::find_if(_code_objects.begin(), _code_objects.end(), hsaco_eq)}; it != _code_objects.end())
        return {std::ref(*it)};

    if (auto reader = std::get_if<hsa_code_object_reader_t>(hsaco))
        _logger->error("Cannot find code object by hsa_code_object_reader_t: " + std::to_string(reader->handle));
    if (auto cobj = std::get_if<hsa_code_object_t>(hsaco))
        _logger->error("Cannot find code object by hsa_code_object_t: " + std::to_string(cobj->handle));

    return {};
}
