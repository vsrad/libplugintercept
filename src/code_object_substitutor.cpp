#include "code_object_substitutor.hpp"
#include <sstream>

using namespace agent;

std::optional<CodeObject> CodeObjectSubstitutor::substitute(hsa_agent_t agent, const RecordedCodeObject& co)
{
    auto sub = std::find_if(_subs.begin(), _subs.end(), [&co](const auto& sub) {
        if (sub.condition_crc && co.crc() != *sub.condition_crc)
            return false;
        if (sub.condition_load_id && co.load_call_id() != *sub.condition_load_id)
            return false;
        return true;
    });
    if (sub != _subs.end())
    {
        _logger.info("Substituting code object " + co.info() + " with " + sub->replacement_path);
        if (auto replacement_co = CodeObject::try_read_from_file(sub->replacement_path.c_str()))
            return replacement_co;
        else
            _logger.error("Unable to load replacement code object from " + sub->replacement_path);
    }
    return {};
}

void CodeObjectSubstitutor::prepare_symbol_substitutes(hsa_agent_t agent)
{
    for (const auto& sub : _symbol_subs)
    {
        if (auto replacement_co = CodeObject::try_read_from_file(sub.replacement_path.c_str()))
        {
            const char* error_callsite;
            hsa_executable_t exec;
            hsa_executable_symbol_t sym{0};
            hsa_status_t status = _co_loader.create_executable(*replacement_co, agent, &exec, &error_callsite);
            if (status == HSA_STATUS_SUCCESS)
                status = _co_loader.find_symbol(agent, exec, sub.replacement_name.c_str(), &sym, &error_callsite);

            if (status == HSA_STATUS_SUCCESS && sym.handle != 0)
                _evaluated_symbol_subs.emplace_back(sub, sym);
            else
                _logger.hsa_error("Unable to load " + sub.replacement_name + " from code object at " + sub.replacement_path, status, error_callsite);
        }
        else
        {
            _logger.error("Unable to load replacement code object from " + sub.replacement_path);
        }
    }
}

std::optional<hsa_executable_symbol_t> CodeObjectSubstitutor::substitute_symbol(
    get_info_call_id_t call_id,
    const RecordedCodeObject& co,
    hsa_executable_symbol_t sym)
{
    for (const auto& [sub, replacement_sym] : _evaluated_symbol_subs)
    {
        if ((sub.condition_get_info_id && call_id != *sub.condition_get_info_id) ||
            (sub.condition_crc && co.crc() != *sub.condition_crc) ||
            (sub.condition_load_id && co.load_call_id() != *sub.condition_load_id))
            continue;
        if (auto it{co.symbols().find(sym.handle)}; it != co.symbols().end())
        {
            if (sub.condition_name && it->second != sub.condition_name)
                continue;
            return replacement_sym;
        }
    }
    return {};
}
