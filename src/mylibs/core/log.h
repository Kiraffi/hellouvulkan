#pragma once

#include <core/general.h>

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
