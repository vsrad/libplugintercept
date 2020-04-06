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

hsa_status_t find_substitute_symbol(hsa_executable_t exec, hsa_executable_symbol_t sym, void* data)
{
    auto [target, target_name] = *reinterpret_cast<std::pair<hsa_executable_symbol_t*, const std::string*>*>(data);

    uint32_t name_len;
    hsa_status_t status = hsa_executable_symbol_get_info(sym, HSA_EXECUTABLE_SYMBOL_INFO_NAME_LENGTH, &name_len);
    if (status != HSA_STATUS_SUCCESS)
        return status;

    std::string current_sym_name(name_len, '\0');
    status = hsa_executable_symbol_get_info(sym, HSA_EXECUTABLE_SYMBOL_INFO_NAME, current_sym_name.data());
    if (status == HSA_STATUS_SUCCESS && current_sym_name == *target_name)
        *target = sym;

    return status;
}

void CodeObjectSubstitutor::prepare_symbol_substitutes(hsa_agent_t agent, const RecordedCodeObject& source, const ext_environment_t& env)
{
    for (const auto& sub : _symbol_subs)
    {
        // TODO: rewrite match condition logic
        if (true) //!sub.condition.matches(source))
            continue;
        if (auto sym{source.symbols().find({})}; sym != source.symbols().end())
        {
            // if (!sub.external_command.empty())
            //     if (!ExternalCommand::run_logged(sub.external_command, env, _logger))
            //         continue;
            if (auto replacement_co = CodeObject::try_read_from_file(sub.replacement_path.c_str()))
            {
                const char* error_callsite;
                hsa_executable_t replacement_exec;
                hsa_executable_symbol_t replacement_sym{0};
                hsa_status_t status = _co_loader.create_executable(*replacement_co, agent, &replacement_exec, &error_callsite);
                if (status != HSA_STATUS_SUCCESS)
                {
                    _logger.hsa_error("Unable to load replacement code object from " + sub.replacement_path, status, error_callsite);
                    continue;
                }

                auto lookup_data = std::make_pair<hsa_executable_symbol_t*, const std::string*>(&replacement_sym, &sub.replacement_name);
                status = hsa_executable_iterate_symbols(replacement_exec, find_substitute_symbol, &lookup_data);
                if (status != HSA_STATUS_SUCCESS || replacement_sym.handle == 0)
                {
                    _logger.hsa_error("Unable to find " + sub.replacement_name + " in the replacement code object", status, "hsa_executable_iterate_symbols");
                    continue;
                }

                _evaluated_symbol_subs[sym->second.handle] = replacement_sym;
            }
            else
            {
                _logger.error("Unable to load replacement code object from " + sub.replacement_path);
            }
        }
    }
}

std::optional<hsa_executable_symbol_t> CodeObjectSubstitutor::substitute_symbol(hsa_executable_symbol_t sym)
{
    if (auto it{_evaluated_symbol_subs.find(sym.handle)}; it != _evaluated_symbol_subs.end())
        return {it->second};
    return {};
}
