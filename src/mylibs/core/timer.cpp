#include "timer.h"

#include <core/general.h>
#include <core/mytypes.h>
#if USE_QUERY_PERFORMANCE
    #include <windows.h>
#elif USE_CHRONO_TIMER

// This include seems quite big for visual studio
//#include <chrono>
#include <thread>
//#include <__msvc_chrono.hpp>

static std::chrono::high_resolution_clock::time_point getTimepointFromChrono(uint64_t t)
{
    std::chrono::high_resolution_clock::time_point tp{ std::chrono::nanoseconds{ t } };
    return tp;
}
#endif

Timer::TimePoint Timer::getTime(ClockType clockType)
{
    #if USE_UNIX_CLOCK_COARSE
        int timerType = int(clockType);
        TimePoint newTime;
        clock_gettime(timerType, &newTime);
    #elif USE_QUERY_PERFORMANCE
        uint64_t newTime;
        LARGE_INTEGER stamp;
        QueryPerformanceCounter(&stamp);
        newTime = stamp.QuadPart;
    #elif USE_CHRONO_TIMER
        std::chrono::high_resolution_clock::time_point chTime = std::chrono::high_resolution_clock::now();
        TimePoint newTime = chTime.time_since_epoch().count();
    #endif
    return newTime;
}

double Timer::getTimeDifferenceInNanos(const TimePoint &fromTime, const TimePoint &toTime)
{
    #if USE_UNIX_CLOCK_COARSE
        double dur = double(toTime.tv_sec - fromTime.tv_sec) +
            double(toTime.tv_nsec - fromTime.tv_nsec) * 1.0e-9;
    #elif USE_CHRONO_TIMER
        
        double dur = (toTime - fromTime) * 1.0e-9;
    #elif USE_QUERY_PERFORMANCE
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    
    double dur = ((toTime - fromTime) * 1.0e6) / double(freq.QuadPart);
    #endif
    return dur;
}


Timer::Timer() : clockType(ClockType::ClockId)
{
    startTime =  Timer::getTime(clockType);
    lastTime = startTime;
}

Timer::Timer(ClockType clockType) : clockType(clockType)
{
    #if USE_UNIX_CLOCK_COARSE
        //TimePoint p;
        //clock_getres(timerType, &p);
        //printf("res: %llu, %llu\n", p.tv_sec, p.tv_nsec);
    #endif
    startTime =  Timer::getTime(clockType);
    lastTime = startTime;

}

double Timer::getDuration()
{
    if(running)
    {
        lastTime = getTime(clockType);
        double dur = Timer::getTimeDifferenceInNanos(startTime, lastTime);
        wholeDuration += dur;
    }
    return wholeDuration;
}

double Timer::getDuration() const
{
    double dur = 0.0;
    if(running)
    {
        TimePoint newTime = getTime(clockType);
        dur = Timer::getTimeDifferenceInNanos(startTime, newTime);
    }
    return wholeDuration + dur;
}


double Timer::getLapDuration()
{
    double dur = 0.0;
    if(running)
    {
        TimePoint newTime = getTime(clockType);
        dur = Timer::getTimeDifferenceInNanos(lastTime, newTime);
        lastTime = newTime;
        wholeDuration += dur;
    }
    return dur;
}

void Timer::continueTimer()
{
    if(running)
        return;
    running = true;
    lastTime = getTime(clockType);
}

void Timer::pauseTimer()
{
    if(!running)
        return;
    running = false;
    TimePoint newTime = getTime(clockType);
    double dur = Timer::getTimeDifferenceInNanos(lastTime, newTime);
    lastTime = newTime;
    wholeDuration += dur;
}

void Timer::resetTimer()
{
    wholeDuration = 0.0;
    startTime = getTime(clockType);
    lastTime = startTime;
}


void printTime(const Timer& timer)
{
    float msTime = (float)timer.getDuration();
    printf("Timer: %fs\n", msTime);
}

void printNamedTimer(const Timer& timer, const char* name)
{
    float msTime = (float)timer.getDuration();
    printf("Timer %s: %f\n", name, msTime);
}
