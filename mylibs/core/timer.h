#pragma once

#include <chrono>

constexpr float billion = 1.0e9f;

class Timer
{
public:
    Timer();
    double getDuration() const;
    double getLapDuration();

private:
    std::chrono::high_resolution_clock::time_point startTime;
    std::chrono::high_resolution_clock::time_point lastTime;
};

void printTime(const Timer& timer);
void printNamedTimer(const Timer& timer, const char* name);

struct ScopedTimer
{
    ScopedTimer(const char *name)
    {
        timerName = name;
    }

    ScopedTimer()
    {
    }

    ~ScopedTimer()
    {
        if(timerName)
            printNamedTimer(timer, timerName);
        else
            printTime(timer);
    }

    Timer timer;
    const char *timerName = nullptr;
};

