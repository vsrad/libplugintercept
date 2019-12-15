#include "CodeObjectSwapper.hpp"
#include <iostream>

using namespace agent;

// TODO: use the logger instead of raw output streams

std::optional<CodeObject> do_swap(const CodeObjectSwapRequest& request)
{
    auto new_co = CodeObject::try_read_from_file(request.replacement_path.c_str());
    if (!new_co)
        std::cerr << "Unable to load the replacement code object from " << request.replacement_path << std::endl;
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
                std::cout << "Code object load #" << *target_call_count << ": swapping for "
                          << swap_request.replacement_path << std::endl;
                return do_swap(swap_request);
            }
        }
        else if (auto target_crc = std::get_if<crc32_t>(&swap_request.condition))
        {
            if (*target_crc == source.CRC())
            {
                std::cout << "Code object load with CRC = " << *target_crc << ": swapping for "
                          << swap_request.replacement_path << std::endl;
                return do_swap(swap_request);
            }
        }

    return {};
}
