#pragma once

#include <ostream>
#include <string>

namespace agent
{
struct BufferAllocation
{
    uint64_t size;
    std::string dump_path;
    std::string addr_env_name;
    std::string size_env_name;

    bool operator!=(const BufferAllocation& rhs) const { return !operator==(rhs); }
    bool operator==(const BufferAllocation& rhs) const
    {
        return size == rhs.size &&
               dump_path == rhs.dump_path &&
               addr_env_name == rhs.addr_env_name &&
               size_env_name == rhs.size_env_name;
    }
    friend std::ostream& operator<<(std::ostream& os, const BufferAllocation& ba)
    {
        os << "{ size = " << ba.size << ", dump_path = " << ba.dump_path
           << ", addr_env_name = " << ba.addr_env_name << ", size_env_name = " << ba.size_env_name << " }";
        return os;
    }
};
} // namespace agent
