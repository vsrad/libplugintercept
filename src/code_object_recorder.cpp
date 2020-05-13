#include "code_object_recorder.hpp"
#include "code_object_loader.hpp"
#include "fs_utils.hpp"
#include <cassert>
#include <cstring>
#include <fstream>
#include <sstream>

using namespace agent;

void CodeObjectRecorder::record_code_object(const void* ptr, size_t size, hsaco_t hsaco, hsa_status_t load_status)
{
    auto load_call_id = ++_load_call_counter;

    std::ostringstream load_signature;
    load_signature << CodeObjectLoader::load_function_name(hsaco) << "(" << ptr << ", " << size << ", ...)";

    if (load_status != HSA_STATUS_SUCCESS)
    {
        _logger.warning("Load #" + std::to_string(load_call_id) + " failed: " + load_signature.str() + " returned " + std::to_string(load_status));
        return;
    }

    auto& code_object = _code_objects.emplace_front(ptr, size, load_call_id, hsaco);
    _logger.info(code_object, load_signature.str());

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
    fs_utils::create_parent_directories(filepath.c_str());
    if (std::ofstream fs{filepath, std::ios::out | std::ios::binary})
    {
        fs.write((char*)co.ptr(), co.size());
        _logger.info(co, "written to " + filepath);
    }
    else
    {
        _logger.error(co, "cannot write code object to " + filepath);
    }
}

void CodeObjectRecorder::handle_crc_collision(const RecordedCodeObject& new_co, const RecordedCodeObject& existing_co)
{
    if (new_co.size() != existing_co.size())
    {
        _logger.error(new_co, "CRC collision with CO " + existing_co.info() +
                                  " (" + std::to_string(new_co.size()) + " bytes new, " + std::to_string(existing_co.size()) + " bytes existing)");
        return;
    }
    // We have a pointer to the contents of the existing code object,
    // but we cannot directly read it because the memory area could've been already freed.
    if (_dump_dir.empty())
    {
        _logger.warning(new_co, "same CRC and size as CO " + existing_co.info() + ". " +
                                    "This is most likely a redundant load. Specify code object dump directory to compare their contents to check for a CRC collision.");
        return;
    }
    // If we dump code objects to disk, we can load the contents of the existing code object
    // and compare them to the new code object.
    auto filepath = existing_co.dump_path(_dump_dir);
    std::ifstream in(filepath, std::ios::binary);
    if (!in)
    {
        _logger.error(new_co, "same CRC and size as CO " + existing_co.info() + ", but their contents could not be compared: could not read " + filepath);
        return;
    }
    char* existing_co_contents = (char*)std::malloc(existing_co.size());
    std::copy(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>(), existing_co_contents);
    if (std::memcmp(new_co.ptr(), existing_co_contents, existing_co.size()))
        _logger.error(new_co, "CRC collision with CO " + existing_co.info() + " (dumped to " + filepath + ")");
    else
        _logger.warning(new_co, "redundant load, same contents as CO " + existing_co.info());
    std::free(existing_co_contents);
}

void CodeObjectRecorder::record_symbols(RecordedCodeObject& co, exec_symbols_t&& symbols)
{
    co.set_symbols(std::move(symbols));
    if (co.symbols().empty())
    {
        _logger.info(co, "no symbols found");
    }
    else
    {
        std::string symbols;
        for (const auto& it : co.symbols())
            symbols.append(symbols.empty() ? "code object symbols: " : ", ").append(it.second);
        _logger.info(co, symbols);
    }
}

std::optional<std::reference_wrapper<RecordedCodeObject>> CodeObjectRecorder::find_code_object(const hsaco_t* hsaco)
{
    auto hsaco_eq = [hsaco](const auto& co) { return co.hsaco_eq(hsaco); };
    if (auto it{std::find_if(_code_objects.begin(), _code_objects.end(), hsaco_eq)}; it != _code_objects.end())
        return {std::ref(*it)};

    if (auto reader = std::get_if<hsa_code_object_reader_t>(hsaco))
        _logger.error("Cannot find code object by hsa_code_object_reader_t: " + std::to_string(reader->handle));
    if (auto cobj = std::get_if<hsa_code_object_t>(hsaco))
        _logger.error("Cannot find code object by hsa_code_object_t: " + std::to_string(cobj->handle));

    return {};
}

std::optional<CodeObjectSymbolInfoCall> CodeObjectRecorder::record_get_info(hsa_executable_symbol_t symbol, hsa_executable_symbol_info_t attribute)
{
    auto call_id = ++_get_info_call_counter;

    for (const auto& co : _code_objects)
    {
        if (auto symbol_it{co.symbols().find(symbol.handle)}; symbol_it != co.symbols().end())
        {
            _logger.info(co, "kernel-get-id " + std::to_string(call_id) + ": hsa_executable_symbol_get_info(\"" +
                                 symbol_it->second + "\", " + std::string(symbol_info_attribute_name(attribute)) + ", ...)");
            return {{.co = &co, .call_id = call_id, .symbol_name = symbol_it->second}};
        }
    }

    _logger.info("kernel-get-id " + std::to_string(call_id) + ": hsa_executable_symbol_get_info(hsa_executable_symbol_t{" +
                 std::to_string(symbol.handle) + "}, " + std::string(symbol_info_attribute_name(attribute)) + ", ...)");
    return {};
}

const char* CodeObjectRecorder::symbol_info_attribute_name(hsa_executable_symbol_info_t attribute)
{
    switch (attribute)
    {
    case HSA_EXECUTABLE_SYMBOL_INFO_TYPE:
        return "HSA_EXECUTABLE_SYMBOL_INFO_TYPE";
    case HSA_EXECUTABLE_SYMBOL_INFO_NAME_LENGTH:
        return "HSA_EXECUTABLE_SYMBOL_INFO_NAME_LENGTH";
    case HSA_EXECUTABLE_SYMBOL_INFO_NAME:
        return "HSA_EXECUTABLE_SYMBOL_INFO_NAME";
    case HSA_EXECUTABLE_SYMBOL_INFO_MODULE_NAME_LENGTH:
        return "HSA_EXECUTABLE_SYMBOL_INFO_MODULE_NAME_LENGTH";
    case HSA_EXECUTABLE_SYMBOL_INFO_MODULE_NAME:
        return "HSA_EXECUTABLE_SYMBOL_INFO_MODULE_NAME";
    case HSA_EXECUTABLE_SYMBOL_INFO_AGENT:
        return "HSA_EXECUTABLE_SYMBOL_INFO_AGENT";
    case HSA_EXECUTABLE_SYMBOL_INFO_VARIABLE_ADDRESS:
        return "HSA_EXECUTABLE_SYMBOL_INFO_VARIABLE_ADDRESS";
    case HSA_EXECUTABLE_SYMBOL_INFO_LINKAGE:
        return "HSA_EXECUTABLE_SYMBOL_INFO_LINKAGE";
    case HSA_EXECUTABLE_SYMBOL_INFO_IS_DEFINITION:
        return "HSA_EXECUTABLE_SYMBOL_INFO_IS_DEFINITION";
    case HSA_EXECUTABLE_SYMBOL_INFO_VARIABLE_ALLOCATION:
        return "HSA_EXECUTABLE_SYMBOL_INFO_VARIABLE_ALLOCATION";
    case HSA_EXECUTABLE_SYMBOL_INFO_VARIABLE_SEGMENT:
        return "HSA_EXECUTABLE_SYMBOL_INFO_VARIABLE_SEGMENT";
    case HSA_EXECUTABLE_SYMBOL_INFO_VARIABLE_ALIGNMENT:
        return "HSA_EXECUTABLE_SYMBOL_INFO_VARIABLE_ALIGNMENT";
    case HSA_EXECUTABLE_SYMBOL_INFO_VARIABLE_SIZE:
        return "HSA_EXECUTABLE_SYMBOL_INFO_VARIABLE_SIZE";
    case HSA_EXECUTABLE_SYMBOL_INFO_VARIABLE_IS_CONST:
        return "HSA_EXECUTABLE_SYMBOL_INFO_VARIABLE_IS_CONST";
    case HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_OBJECT:
        return "HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_OBJECT";
    case HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_KERNARG_SEGMENT_SIZE:
        return "HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_KERNARG_SEGMENT_SIZE";
    case HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_KERNARG_SEGMENT_ALIGNMENT:
        return "HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_KERNARG_SEGMENT_ALIGNMENT";
    case HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_GROUP_SEGMENT_SIZE:
        return "HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_GROUP_SEGMENT_SIZE";
    case HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_PRIVATE_SEGMENT_SIZE:
        return "HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_PRIVATE_SEGMENT_SIZE";
    case HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_DYNAMIC_CALLSTACK:
        return "HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_DYNAMIC_CALLSTACK";
    case HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_CALL_CONVENTION:
        return "HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_CALL_CONVENTION";
    case HSA_EXECUTABLE_SYMBOL_INFO_INDIRECT_FUNCTION_OBJECT:
        return "HSA_EXECUTABLE_SYMBOL_INFO_INDIRECT_FUNCTION_OBJECT";
    case HSA_EXECUTABLE_SYMBOL_INFO_INDIRECT_FUNCTION_CALL_CONVENTION:
        return "HSA_EXECUTABLE_SYMBOL_INFO_INDIRECT_FUNCTION_CALL_CONVENTION";
    default:
        return "?";
    }
}
