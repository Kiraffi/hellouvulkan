#ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wtautological-compare" // comparison of unsigned expression < 0 is always false
    #pragma clang diagnostic ignored "-Wunused-private-field"
    #pragma clang diagnostic ignored "-Wunused-parameter"
    #pragma clang diagnostic ignored "-Wmissing-field-initializers"
    #pragma clang diagnostic ignored "-Wnullability-completeness"
#endif

#define VMA_IMPLEMENTATION
#define VMA_STATS_STRING_ENABLED 0
//#define VMA_BIND_MEMORY2 0
//#define VMA_MEMORY_PRIORITY 0
//#define VMA_EXTERNAL_MEMORY 0
//#define VMA_BUFFER_DEVICE_ADDRESS 0
#include <vk_mem_alloc.h>

#ifdef __clang__
    #pragma clang diagnostic pop
#endif
