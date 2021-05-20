#pragma once

#include <assert.h>
#include <stdio.h>

#include <stdint.h>

using f32 = float;
using f64 = double;
using u32 = unsigned int;
using i32 = int;
using b32 = int;

using u8 = unsigned char;
using i8 = char;
using u16 = unsigned short;
using i16 = short;
using u64 = uint64_t;
using i64 = int64_t;


const float pii = 3.1415926265359f;



#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

#ifdef NDEBUG
	#define VK_CHECK(call) do { [[maybe_unused]] VkResult callResult = call ;} while(0)
#else
	#define VK_CHECK(call) do { VkResult callResult = call; assert(callResult == VkResult::VK_SUCCESS); } while(0)
#endif

#ifndef ARRAYSIZE
#define ARRAYSIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#endif

#if 0
#ifdef NDEBUG
	#define ASSERT(assertcall) do { } while(0)
#else
	#define ASSERT(assertcall) do{ assert(assertcall); } while(0)
#endif
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
					fprintf(stdout, LOG_TEXT_STRING, ##__VA_ARGS__); \
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

