#include "CodeObjectSwapper.hpp"
#include "external_command.hpp"
#include <sstream>

using namespace agent;

std::optional<CodeObject> CodeObjectSwapper::get_swapped_code_object(std::shared_ptr<CodeObject> source, const std::unique_ptr<Buffer>& debug_buffer)
{
    call_count_t current_call = ++_call_counter;

    for (auto& swap : *_swaps)
        if (auto target_call_count = std::get_if<call_count_t>(&swap.condition))
        {
            if (*target_call_count == current_call)
            {
                _logger->info(
                    "Code object load #" + std::to_string(*target_call_count) + ": swapping for " + swap.replacement_path);
                return do_swap(swap, source, debug_buffer);
            }
        }
        else if (auto target_crc = std::get_if<crc32_t>(&swap.condition))
        {
            if (*target_crc == source->CRC())
            {
                _logger->info(
                    "Code object load with CRC = " + std::to_string(*target_crc) + ": swapping for " + swap.replacement_path);
                return do_swap(swap, source, debug_buffer);
            }
        }

    return {};
}

std::optional<CodeObject> CodeObjectSwapper::do_swap(const CodeObjectSwap& swap, std::shared_ptr<CodeObject> source, const std::unique_ptr<Buffer>& debug_buffer)
{
    if (!swap.external_command.empty())
    {
        _logger->info("Executing `" + swap.external_command + "`");
        ExternalCommand cmd(swap.external_command);
        std::map<std::string, std::string> environment;

        if (debug_buffer)
        {
            environment["ASM_DBG_BUF_SIZE"] = std::to_string(debug_buffer->Size());
            environment["ASM_DBG_BUF_ADDR"] = std::to_string(reinterpret_cast<size_t>(debug_buffer->LocalPtr()));
        }
        else
        {
            _logger->info("ASM_DBG_BUF_SIZE and ASM_DBG_BUF_ADDR are not set because the debug buffer has not been allocated yet.");
        }

        int retcode = cmd.execute(environment);
        if (retcode != 0)
        {
            std::ostringstream error_log;
            error_log << "The command `" << swap.external_command << "` has exited with code " << retcode;
            auto stdout = cmd.read_stdout();
            if (!stdout.empty())
                error_log << "\n=== Stdout:\n"
                          << stdout;
            auto stderr = cmd.read_stderr();
            if (!stderr.empty())
                error_log << "\n=== Stderr:\n"
                          << stderr;
            _logger->error(error_log.str());
            return {};
        }
        _logger->info("The command has finished successfully");
    }

    auto new_co = CodeObject::try_read_from_file(swap.replacement_path.c_str());
    if (!new_co)
        _logger->error("Unable to load the replacement code object from " + swap.replacement_path);

    if (swap.symbol_swaps.empty())
        return new_co;

    _symbol_swaps.emplace(source, SymbolSwap{.swap = swap, .replacement_co = *new_co, .exec = {0}});
    return {};
}

hsa_status_t create_executable(CodeObject& co, hsa_agent_t agent, hsa_executable_t* executable, const char** error_callsite)
{
    hsa_code_object_reader_t co_reader;
    hsa_status_t status = hsa_code_object_reader_create_from_memory(co.Ptr(), co.Size(), &co_reader);
    if (status != HSA_STATUS_SUCCESS)
    {
        *error_callsite = "hsa_code_object_reader_create_from_memory";
        return status;
    }
    status = hsa_executable_create(HSA_PROFILE_FULL, HSA_EXECUTABLE_STATE_UNFROZEN, NULL, executable);
    if (status != HSA_STATUS_SUCCESS)
    {
        *error_callsite = "hsa_executable_create";
        return status;
    }
    status = hsa_executable_load_agent_code_object(*executable, agent, co_reader, NULL, NULL);
    if (status != HSA_STATUS_SUCCESS)
    {
        *error_callsite = "hsa_executable_load_agent_code_object";
        return status;
    }
    status = co.fill_symbols(*executable);
    if (status != HSA_STATUS_SUCCESS)
        *error_callsite = "hsa_executable_iterate_symbols";
    return status;
}

void CodeObjectSwapper::prepare_symbol_swap(std::shared_ptr<CodeObject> source, hsa_agent_t agent)
{
    if (auto it{_symbol_swaps.find(source)}; it != _symbol_swaps.end())
    {
        auto& swap = it->second;
        const char *error_site, *err;
        hsa_status_t status = create_executable(swap.replacement_co, agent, &swap.exec, &error_site);
        if (status != HSA_STATUS_SUCCESS)
        {
            hsa_status_string(status, &err);
            _logger->error(std::string("Unable to prepare replacement code object for CRC = ")
                               .append(std::to_string(source->CRC()))
                               .append(": ")
                               .append(error_site)
                               .append(" failed with ")
                               .append(err));
            swap.exec = {0};
        }
    }
}

std::optional<hsa_executable_symbol_t> CodeObjectSwapper::swap_symbol(std::shared_ptr<CodeObject> source, const std::string& source_symbol_name)
{
    if (auto it{_symbol_swaps.find(source)}; it != _symbol_swaps.end())
    {
        auto src_rep_it = std::find_if(it->second.swap.symbol_swaps.begin(), it->second.swap.symbol_swaps.end(),
                                       [name = source_symbol_name](const auto& src_rep_names) { return src_rep_names.first == name; });
        if (src_rep_it != it->second.swap.symbol_swaps.end())
        {
            auto& replacement_symbols = it->second.replacement_co.symbols();
            auto sym_it = std::find_if(replacement_symbols.begin(), replacement_symbols.end(),
                                       [name = src_rep_it->second](const auto& src_rep_names) { return src_rep_names.second == name; });
            if (sym_it != replacement_symbols.end())
                return {hsa_executable_symbol_t{sym_it->first}};
        }
    }
    return {};
}
