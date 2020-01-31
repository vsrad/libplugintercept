#pragma once

#include "code_object_loader.hpp"
#include "code_object_recorder.hpp"
#include "code_object_swapper.hpp"
#include "config.hpp"
#include "debug_buffer.hpp"
#include "logger/logger.hpp"
#include <memory>
#include <string>

namespace agent
{
class DebugAgent
{
private:
    hsa_region_t _gpu_local_region;
    hsa_region_t _system_region;
    std::shared_ptr<Config> _config;
    std::shared_ptr<AgentLogger> _logger;
    std::unique_ptr<CodeObjectLoader> _co_loader;
    std::unique_ptr<CodeObjectRecorder> _co_recorder;
    std::unique_ptr<CodeObjectSwapper> _co_swapper;
    std::unique_ptr<DebugBuffer> _debug_buffer;

    template <typename T>
    std::optional<T> load_swapped_code_object(hsa_agent_t agent, RecordedCodeObject& co);

public:
    DebugAgent(std::shared_ptr<Config> config,
               std::shared_ptr<AgentLogger> logger,
               std::shared_ptr<CodeObjectLogger> co_logger,
               std::unique_ptr<CodeObjectLoader> co_loader)
        : _gpu_local_region{0}, _system_region{0}, _config(config), _logger(logger),
          _co_loader(std::move(co_loader)),
          _co_recorder(std::make_unique<CodeObjectRecorder>(config->code_object_dump_dir(), co_logger)),
          _co_swapper(std::make_unique<CodeObjectSwapper>(config->code_object_swaps(), logger)) {}

    ~DebugAgent() noexcept { write_debug_buffer_to_file(); }

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

    hsa_status_t intercept_hsa_code_object_deserialize(
        decltype(hsa_code_object_deserialize)* intercepted_fn,
        void* serialized_code_object,
        size_t serialized_code_object_size,
        const char* options,
        hsa_code_object_t* code_object);

    hsa_status_t intercept_hsa_executable_load_agent_code_object(
        decltype(hsa_executable_load_agent_code_object)* intercepted_fn,
        hsa_executable_t executable,
        hsa_agent_t agent,
        hsa_code_object_reader_t code_object_reader,
        const char* options,
        hsa_loaded_code_object_t* loaded_code_object);

    hsa_status_t intercept_hsa_executable_load_code_object(
        decltype(hsa_executable_load_code_object)* intercepted_fn,
        hsa_executable_t executable,
        hsa_agent_t agent,
        hsa_code_object_t code_object,
        const char* options);

    hsa_status_t intercept_hsa_executable_symbol_get_info(
        decltype(hsa_executable_symbol_get_info)* intercepted_fn,
        hsa_executable_symbol_t executable_symbol,
        hsa_executable_symbol_info_t attribute,
        void* value);
};

} // namespace agent
