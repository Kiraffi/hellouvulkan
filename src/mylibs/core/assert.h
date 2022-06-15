#pragma once

#include <core/log.h>

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

