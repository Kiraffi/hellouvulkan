#include "timer.h"

#include <core/general.h>
#include <core/mytypes.h>


// This include seems quite big for visual studio
#include <chrono>

static std::chrono::high_resolution_clock::time_point getTimepointFromChrono(uint64_t t)
{
    std::chrono::high_resolution_clock::time_point tp{ std::chrono::nanoseconds{ t } };
    return tp;
}

Timer::TimePoint Timer::getTime(ClockType clockType)
{
    #if USE_UNIX_CLOCK_COARSE
        int timerType = int(clockType);
        TimePoint newTime;
        clock_gettime(timerType, &newTime);
    #else
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
    #else
        double dur = (toTime - fromTime) * 1.0e-9;
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
