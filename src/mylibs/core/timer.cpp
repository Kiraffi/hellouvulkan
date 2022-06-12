#include "timer.h"
#include <core/mytypes.h>


TimePoint Timer::getTime()
{
    #if USE_UNIX_CLOCK_COARSE
        TimePoint newTime;
        clock_gettime(clockIdt, &newTime);
    #else
        TimePoint newTime = std::chrono::high_resolution_clock::now();
    #endif
    return newTime;
}

double Timer::getTimeDifferenceInNanos(const TimePoint &fromTime, const TimePoint &toTime)
{
    #if USE_UNIX_CLOCK_COARSE
        double dur = double(toTime.tv_sec - fromTime.tv_sec) +
            double(toTime.tv_nsec - fromTime.tv_nsec) * 1.0e-9;
    #else
        double dur = std::chrono::duration_cast<std::chrono::nanoseconds>
            (toTime - fromTime).count() * 1.0e-9;
    #endif
    return dur;
}


Timer::Timer()
{
    #if USE_UNIX_CLOCK_COARSE
        TimePoint p;
        clock_getres(clockIdt, &p);
        printf("res: %llu, %llu\n", p.tv_sec, p.tv_nsec);
    #endif
    startTime =  Timer::getTime();
    lastTime = startTime;
}

double Timer::getDuration()
{
    if(running)
    {
        lastTime = getTime();
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
        TimePoint newTime = getTime();
        dur = Timer::getTimeDifferenceInNanos(startTime, newTime);
    }
    return wholeDuration + dur;
}


double Timer::getLapDuration()
{
    double dur = 0.0;
    if(running)
    {
        TimePoint newTime = getTime();
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
    lastTime = getTime();
}

void Timer::pauseTimer()
{
    if(!running)
        return;
    running = false;
    TimePoint newTime = getTime();
    double dur = Timer::getTimeDifferenceInNanos(lastTime, newTime);
    lastTime = newTime;
    wholeDuration += dur;
}

void Timer::resetTimer()
{
    wholeDuration = 0.0;
    startTime = getTime();
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
