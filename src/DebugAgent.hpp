#pragma once

#define AMD_INTERNAL_BUILD
#include <hsa_api_trace.h>

#include "CodeObjectManager.hpp"
#include "CodeObjectSwapper.hpp"
#include "config.hpp"
#include "logger/logger.hpp"
#include <memory>
#include <string>

namespace agent
{

class Buffer
{
private:
    size_t size;
    void *local_ptr, *system_ptr;

public:
    Buffer(size_t size_, void* local_ptr_, void* system_ptr_)
        : size(size_), local_ptr(local_ptr_), system_ptr(system_ptr_) {}
    void* LocalPtr() { return local_ptr; }
    void* SystemPtr() { return system_ptr; }
    size_t Size() const { return size; }
};

class DebugAgent
{
private:
    hsa_region_t _gpu_local_region;
    hsa_region_t _system_region;
    std::shared_ptr<Config> _config;
    std::shared_ptr<Logger> _logger;
    std::unique_ptr<CodeObjectManager> _code_object_manager;
    std::unique_ptr<CodeObjectSwapper> _code_object_swapper;
    std::unique_ptr<Buffer> _debug_buffer;

public:
    DebugAgent(std::shared_ptr<Config> config, std::shared_ptr<Logger> logger)
        : _gpu_local_region{0}, _system_region{0}, _config(config), _logger(logger),
          _code_object_manager(std::make_unique<CodeObjectManager>(config->code_object_dump_dir(), config->code_object_log_file())),
          _code_object_swapper(std::make_unique<CodeObjectSwapper>(config->code_object_swaps(), logger)) {}

    hsa_region_t gpu_region() const { return _gpu_local_region; }
    hsa_region_t system_region() const { return _system_region; }

    void set_gpu_region(hsa_region_t region) { _gpu_local_region = region; }
    void set_system_region(hsa_region_t region) { _system_region = region; }

    void write_debug_buffer_to_file();

    hsa_status_t intercept_hsa_code_object_reader_create_from_memory(
        decltype(hsa_code_object_reader_create_from_memory)* intercepted_fn,
        const void* code_object,
        size_t size,
        hsa_code_object_reader_t* code_object_reader);

    hsa_status_t intercept_hsa_queue_create(
        decltype(hsa_queue_create)* intercepted_fn,
        hsa_agent_t agent,
        uint32_t size,
        hsa_queue_type32_t type,
        void (*callback)(hsa_status_t status, hsa_queue_t* source, void* data),
        void* data,
        uint32_t private_segment_size,
        uint32_t group_segment_size,
        hsa_queue_t** queue);
};

} // namespace agent
