#pragma once

#include <ostream>
#include <string>

namespace agent::config
{
struct TrapHandler
{
    std::string code_object_path;
    std::string symbol_name;
    uint64_t buffer_size;
    std::string buffer_dump_path;

    bool operator!=(const TrapHandler& rhs) const { return !operator==(rhs); }
    bool operator==(const TrapHandler& rhs) const
    {
        return code_object_path == rhs.code_object_path && symbol_name == rhs.symbol_name
            && buffer_size == rhs.buffer_size && buffer_dump_path == rhs.buffer_dump_path;
    }

    friend std::ostream& operator<<(std::ostream& os, const TrapHandler& th)
    {
        os << "{ code_object_path = " << th.code_object_path << ", symbol_name = " << th.symbol_name
           << ", buffer_size = " << th.buffer_size << ", buffer_dump_path =" << th.buffer_dump_path << " }";
        return os;
    }
};
} // namespace agent::config
