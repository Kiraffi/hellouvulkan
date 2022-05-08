#pragma once

#include <stdint.h>

void myPrint(const char* ptr, ...);

extern "C" int printf (const char *__restrict __format, ...);
//extern "C" void *memcpy(void *__restrict dest, const void *__restrict src, int64_t n);
//void *memcpy(void * dest, const void * src, int64_t n);
//extern "C" int memcmp( const void *ptr1, const void *ptr2, long num );
//extern "C" int strcmp(const char *str1, const char *str2);

float ftanf(float a);
float fsinf(float a);
float fcosf(float a);
float ffabsf(float a);
float fsqrtf(float a);
float ffmodf(float a, float b);
float ffminf(float a, float b);
float ffmaxf(float a, float b);

const float pii = 3.1415926265359f;



#if USE_GPU_DEBUG_VALIDATION
    const bool enableValidationLayers = true;
#else
    const bool enableValidationLayers = false;
#endif









static constexpr float PI = 3.14159265358979323846264338f;

void printOutBuffer(const char *buffer);

#define LOGDEBUG 1
#if LOGDEBUG == 1
#define LOG_LEN 1024
#if 0 // _MSC_VER
#define HELPER_LOG(LOG_TYPE, LOG_TEXT_STRING, ...) \
                do { \
                    char debugBuffer[LOG_LEN]; \
                    snprintf(debugBuffer, (size_t)LOG_LEN, LOG_TYPE ## LOG_TEXT_STRING, ##__VA_ARGS__); \
                    printOutBuffer(debugBuffer); \
                } while(0)
#else
#define HELPER_LOG(LOG_TYPE, LOG_TEXT_STRING, ...) \
                do { \
                    printf(LOG_TEXT_STRING, ##__VA_ARGS__); \
                } while(0)
#endif

#else
#define HELPER_LOG(LOG_TYPE, LOG_TEXT_STRING, ...)
#endif

#define PRINTF(LOG_TEXT_STRING, ...) \
                do { \
                    char debugBuffer[LOG_LEN]; \
                    snprintf(debugBuffer, (size_t)LOG_LEN, LOG_TEXT_STRING, ##__VA_ARGS__); \
                    printOutBuffer(debugBuffer); \
                } while(0)

#define LOG(LOG_TEXT_STRING, ...) HELPER_LOG("Output: ", LOG_TEXT_STRING, ##__VA_ARGS__)
#define LOGERROR(LOG_TEXT_STRING, ...) HELPER_LOG("Error: ", LOG_TEXT_STRING, ##__VA_ARGS__)

#if _MSC_VER
#define DEBUG_BREAK_MACRO() __debugbreak()
#else
#include <signal.h>
    //#define DEBUG_BREAK_MACRO __builtin_trap()
#define DEBUG_BREAK_MACRO() raise(SIGTRAP)
#endif

#define HELPER_ASSERT_STRING(STUFF, STUFFSTRING) \
do \
{ \
    if (STUFF) {} \
    else  \
    {  \
        LOG("Assertion: %s\n", STUFFSTRING); \
        DEBUG_BREAK_MACRO(); \
    } \
} while (0)

#define ASSERT(STUFF) HELPER_ASSERT_STRING(STUFF, #STUFF)




#ifdef NDEBUG
    #define VK_CHECK(call) do { [[maybe_unused]] VkResult callResult = call ;} while(0)
#else
    #define VK_CHECK(call) do { VkResult callResult = call; ASSERT(callResult == VkResult::VK_SUCCESS); } while(0)
#endif

#ifndef ARRAYSIZES
    #define ARRAYSIZES(arr) (sizeof(arr) / sizeof(arr[0]))
#endif
