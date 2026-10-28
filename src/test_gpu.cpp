#include <cstdio>
#include "net.h"
#include "gpu.h"

int main()
{
#if NCNN_VULKAN
    int count = ncnn::get_gpu_count();
    printf("GPUs detectadas por ncnn: %d\n", count);
    for (int i = 0; i < count; i++)
    {
        const ncnn::VulkanDevice* gpu = ncnn::get_gpu_device(i);
        if (gpu)
            printf("GPU %d: %s\n", i, gpu->info.device_name());
    }
    ncnn::destroy_gpu_instance();
    return count > 0 ? 0 : 1;
#else
    printf("ERROR: ncnn fue compilado sin Vulkan.\n");
    return 1;
#endif
}
