#include "CodeObjectSwapper.hpp"
#include "external_command.hpp"

using namespace agent;

std::optional<CodeObject> do_swap(const CodeObjectSwapRequest& request, Logger& logger)
{
    if (!request.external_command.empty())
    {
        logger.info("Executing `" + request.external_command + "`");
        ExternalCommand cmd(request.external_command);
        int retcode = cmd.execute();
        if (retcode != 0)
        {
            logger.error("The command `" + request.external_command + "` has exited with code " + std::to_string(retcode));
            auto stdout = cmd.read_stdout();
            if (!stdout.empty())
                logger.error("=== Stdout:\n" + stdout);
            auto stderr = cmd.read_stderr();
            if (!stderr.empty())
                logger.error("=== Stderr:\n" + stderr);
            return {};
        }
        logger.info("The command has finished successfully");
    }

    auto new_co = CodeObject::try_read_from_file(request.replacement_path.c_str());
    if (!new_co)
        logger.error("Unable to load the replacement code object from " + request.replacement_path);
    return new_co;
}

std::optional<CodeObject> CodeObjectSwapper::get_swapped_code_object(const CodeObject& source)
{
    call_count_t current_call = ++_call_counter;

    for (auto& swap_request : _swap_requests)
        if (auto target_call_count = std::get_if<call_count_t>(&swap_request.condition))
        {
            if (*target_call_count == current_call)
            {
                _logger->info(
                    "Code object load #" + std::to_string(*target_call_count) + ": swapping for " + swap_request.replacement_path);
                return do_swap(swap_request, *_logger);
            }
        }
        else if (auto target_crc = std::get_if<crc32_t>(&swap_request.condition))
        {
            if (*target_crc == source.CRC())
            {
                _logger->info(
                    "Code object load with CRC = " + std::to_string(*target_crc) + ": swapping for " + swap_request.replacement_path);
                return do_swap(swap_request, *_logger);
            }
        }

    return {};
}
