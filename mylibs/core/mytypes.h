#pragma once

#include <assert.h>

using f32 = float;
using f64 = double;
using u32 = unsigned int;
using i32 = int;
using b32 = int;

using u8 = unsigned char;
using i8 = char;
using u16 = unsigned short;
using i16 = short;

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


#ifdef NDEBUG
	#define ASSERT(assertcall) do { } while(0)
#else
	#define ASSERT(assertcall) do{ assert(assertcall); } while(0)
#endif