#pragma once

#include "code_object.hpp"
#include "logger/logger.hpp"
#include <shared_mutex>
#include <forward_list>
#include <functional>

namespace agent
{
class CodeObjectRecorder
{
private:
    load_call_id_t _load_call_counter;
    // Note: using a linked list instead of a vector because we pass references to items
    // and we don't want them to be invalidated on insertion.
    std::forward_list<RecordedCodeObject> _code_objects;
    std::string _dump_dir;
    std::shared_ptr<CodeObjectLogger> _logger;
    std::shared_mutex _mutex;

    void dump_code_object(const RecordedCodeObject& co);
    void handle_crc_collision(const RecordedCodeObject& new_co, const RecordedCodeObject& existing_co);

public:
    CodeObjectRecorder(std::string dump_dir, std::shared_ptr<CodeObjectLogger> logger)
        : _load_call_counter{0}, _dump_dir{dump_dir}, _logger{logger} {}
    void record_code_object(const void* ptr, size_t size, hsaco_t hsaco, hsa_status_t load_status);
    std::optional<std::reference_wrapper<RecordedCodeObject>> find_code_object(const hsaco_t* hsaco);
    void iterate_symbols(hsa_executable_t exec, RecordedCodeObject& code_object);
};
} // namespace agent
