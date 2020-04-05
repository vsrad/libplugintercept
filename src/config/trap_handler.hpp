#pragma once

#include <ostream>
#include <string>

namespace agent::config
{
struct TrapHandler
{
    std::string code_object_path;
    std::string symbol_name;

    bool operator!=(const TrapHandler& rhs) const { return !operator==(rhs); }
    bool operator==(const TrapHandler& rhs) const
    {
        return code_object_path == rhs.code_object_path && symbol_name == rhs.symbol_name;
    }

    friend std::ostream& operator<<(std::ostream& os, const TrapHandler& th)
    {
        os << "{ code_object_path = " << th.code_object_path << ", symbol_name = " << th.symbol_name << " }";
        return os;
    }
};
} // namespace agent::config
