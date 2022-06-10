#include "timer.h"
#include <core/mytypes.h>

Timer::Timer()
{
    startTime = std::chrono::high_resolution_clock::now();
    lastTime = startTime;
}

double Timer::getDuration() 
{
    if(running)
    {
        std::chrono::high_resolution_clock::time_point newTime = std::chrono::high_resolution_clock::now();

        double dur = std::chrono::duration_cast<std::chrono::nanoseconds>
            (newTime - startTime).count() * 1.0e-9;

        wholeDuration += dur;
    }
    return wholeDuration;
}

double Timer::getDuration() const
{
    double dur = 0.0;
    if(running)
    {
        std::chrono::high_resolution_clock::time_point newTime = std::chrono::high_resolution_clock::now();

        dur = std::chrono::duration_cast<std::chrono::nanoseconds>
            (newTime - startTime).count() * 1.0e-9;
    }
    return wholeDuration + dur;
}


double Timer::getLapDuration()
{
    double dur = 0.0;
    if(running)
    {
        std::chrono::high_resolution_clock::time_point newTime = std::chrono::high_resolution_clock::now();

        dur = std::chrono::duration_cast<std::chrono::nanoseconds>
            (newTime - lastTime).count() * 1.0e-9;
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
    lastTime = std::chrono::high_resolution_clock::now();
}

void Timer::pauseTimer()
{
    if(!running)
        return;
    running = false;
    std::chrono::high_resolution_clock::time_point newTime = std::chrono::high_resolution_clock::now();

    double dur = std::chrono::duration_cast<std::chrono::nanoseconds>
        (newTime - lastTime).count() * 1.0e-9;
    lastTime = newTime;

    wholeDuration += dur;
}

void Timer::resetTimer()
{
    wholeDuration = 0.0;
    lastTime = std::chrono::high_resolution_clock::now();
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
