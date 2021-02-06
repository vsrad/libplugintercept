#pragma once

#include "buffer_manager.hpp"
#include "code_object_loader.hpp"
#include "code_object_recorder.hpp"
#include "code_object_substitutor.hpp"
#include "config/config.hpp"
#include "logger/logger.hpp"
#include "trap_handler.hpp"

namespace agent
{
class DebugAgent
{
private:
    config::Config _config;
    AgentLogger _logger;
    CodeObjectLogger _co_logger;
    CodeObjectLoader _co_loader;
    CodeObjectRecorder _co_recorder;
    CodeObjectSubstitutor _co_substitutor;
    BufferManager _buffer_manager;
    TrapHandler _trap_handler;

    std::mutex _agent_mutex;
    bool _init_command_executed{false};

    void execute_init_command_once(hsa_agent_t agent);

public:
    DebugAgent(CodeObjectLoader&& co_loader)
        : _config(),
          _logger(_config.agent_log_file()),
          _co_logger(_config.code_object_log_file()),
          _co_loader(std::move(co_loader)),
          _co_recorder(_config.code_object_dump_dir(), _co_logger),
          _co_substitutor(_config.code_object_subs(), _config.symbol_subs(), _logger, _co_loader),
          _buffer_manager(_config.buffers(), _logger),
          _trap_handler(_logger, _co_loader, _config.trap_handler()) {}

    void record_queue_creation(hsa_agent_t agent);

    void record_co_load(
        hsaco_t hsaco,
        const void* contents,
        size_t size,
        hsa_status_t load_status);

    hsa_status_t executable_load_co(
        hsaco_t hsaco,
        hsa_agent_t agent,
        hsa_executable_t executable,
        std::function<hsa_status_t(hsaco_t)> loader);

    hsa_executable_symbol_t symbol_get_info(
        hsa_executable_symbol_t symbol,
        hsa_executable_symbol_info_t attribute);
};
} // namespace agent
