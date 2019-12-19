#include "CodeObjectSwapper.hpp"
#include "external_command.hpp"
#include <sstream>

using namespace agent;

std::optional<CodeObject> do_swap(const CodeObjectSwap& swap, Logger& logger)
{
    if (!swap.external_command.empty())
    {
        logger.info("Executing `" + swap.external_command + "`");
        ExternalCommand cmd(swap.external_command);
        int retcode = cmd.execute();
        if (retcode != 0)
        {
            std::ostringstream error_log;
            error_log << "The command `" << swap.external_command << "` has exited with code " << retcode;
            auto stdout = cmd.read_stdout();
            if (!stdout.empty())
                error_log << "\n=== Stdout:\n" << stdout;
            auto stderr = cmd.read_stderr();
            if (!stderr.empty())
                error_log << "\n=== Stderr:\n" << stderr;
            logger.error(error_log.str());
            return {};
        }
        logger.info("The command has finished successfully");
    }

    auto new_co = CodeObject::try_read_from_file(swap.replacement_path.c_str());
    if (!new_co)
        logger.error("Unable to load the replacement code object from " + swap.replacement_path);
    return new_co;
}

std::optional<CodeObject> CodeObjectSwapper::get_swapped_code_object(const CodeObject& source)
{
    call_count_t current_call = ++_call_counter;

    for (auto& swap : _swaps)
        if (auto target_call_count = std::get_if<call_count_t>(&swap.condition))
        {
            if (*target_call_count == current_call)
            {
                _logger->info(
                    "Code object load #" + std::to_string(*target_call_count) + ": swapping for " + swap.replacement_path);
                return do_swap(swap, *_logger);
            }
        }
        else if (auto target_crc = std::get_if<crc32_t>(&swap.condition))
        {
            if (*target_crc == source.CRC())
            {
                _logger->info(
                    "Code object load with CRC = " + std::to_string(*target_crc) + ": swapping for " + swap.replacement_path);
                return do_swap(swap, *_logger);
            }
        }

    return {};
}
