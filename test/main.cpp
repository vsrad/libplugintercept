#include <fstream>
#include <hsa.h>
#include <iostream>
#include <unistd.h>

hsa_code_object_t code_object;

void hsa_error(const char* msg, hsa_status_t status)
{
    const char* err = 0;
    if (status != HSA_STATUS_SUCCESS)
        hsa_status_string(status, &err);
    std::cout << msg << ": " << (err ? err : "unknown error") << std::endl;
    exit(1);
}

void load_code_object(hsa_agent_t hsa_agent)
{
    const char* filename = "build/test/kernels/dbg_kernel.co";
    std::ifstream in(filename, std::ios::binary | std::ios::ate);
    if (!in)
    {
        std::cerr << "Error: failed to load " << filename << std::endl;
        return;
    }

    size_t size = std::string::size_type(in.tellg());
    char* ptr = (char*)malloc(size);
    if (!ptr)
    {
        std::cerr << "Error: failed to allocate memory for code object" << std::endl;
        return;
    }
    in.seekg(0, std::ios::beg);
    std::copy(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>(), ptr);

    hsa_code_object_reader_t co_reader = {};
    hsa_status_t status = hsa_code_object_reader_create_from_memory(ptr, size, &co_reader);
    if (status != HSA_STATUS_SUCCESS)
        hsa_error("failed to deserialize code object", status);

    hsa_executable_t executable;
    status = hsa_executable_create(HSA_PROFILE_FULL, HSA_EXECUTABLE_STATE_UNFROZEN, NULL, &executable);
    if (status != HSA_STATUS_SUCCESS)
        hsa_error("hsa_executable_create failed", status);

    status = hsa_executable_load_agent_code_object(executable, hsa_agent, co_reader, NULL, NULL);
    if (status != HSA_STATUS_SUCCESS)
        hsa_error("hsa_executable_load_agent_code_object failed", status);
}

hsa_status_t find_gpu_device(hsa_agent_t agent, void* data)
{
    hsa_device_type_t dev;
    hsa_status_t status = hsa_agent_get_info(agent, HSA_AGENT_INFO_DEVICE, &dev);
    if (status == HSA_STATUS_SUCCESS && dev == HSA_DEVICE_TYPE_GPU)
        *static_cast<hsa_agent_t*>(data) = agent;
    return HSA_STATUS_SUCCESS;
}

int main(void)
{
    const char* tools = getenv("HSA_TOOLS_LIB");
    std::cout << tools << std::endl;

    setenv("ASM_DBG_BUF_SIZE", "4194304", 1);

    hsa_status_t status = hsa_init();
    if (status != HSA_STATUS_SUCCESS)
        hsa_error("hsa_init failed", status);

    hsa_agent_t gpu_agent;
    status = hsa_iterate_agents(find_gpu_device, &gpu_agent);
    if (status != HSA_STATUS_SUCCESS)
        hsa_error("hsa_iterate_agents failed", status);

    uint32_t queue_size;
    status = hsa_agent_get_info(gpu_agent, HSA_AGENT_INFO_QUEUE_MAX_SIZE, &queue_size);
    if (status != HSA_STATUS_SUCCESS)
        hsa_error("Failed to get HSA_AGENT_INFO_QUEUE_MAX_SIZE", status);

    hsa_queue_t* queue;
    status = hsa_queue_create(gpu_agent, queue_size, HSA_QUEUE_TYPE_MULTI, NULL, NULL, UINT32_MAX, UINT32_MAX, &queue);
    if (status != HSA_STATUS_SUCCESS)
        hsa_error("hsa_queue_create failed", status);

    load_code_object(gpu_agent);
    return 0;
}
