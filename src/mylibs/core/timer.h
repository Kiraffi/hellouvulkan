#pragma once

#include <chrono>
#include <time.h>

#if __unix__
    #define USE_UNIX_CLOCK_COARSE 1
#endif

// in linux the std::chrono::high_resolution_clock::now(); seems a lot slower call,
// than using coarse if doing a lot of calls. Although it might also give less accurate values.
#if USE_UNIX_CLOCK_COARSE
    // using timespec + CLOCK_MONOTONIC_COARSE
    using TimePoint = timespec;
    #define clockIdt CLOCK_MONOTONIC_COARSE // spammable, but might be less inaccurate
    //#define clockIdt CLOCK_REALTIME_COARSE
    //#define clockIdt CLOCK_THREAD_CPUTIME_ID //, not as slow but still slower, takes hit to spam this
    //#define clockIdt CLOCK_PROCESS_CPUTIME_ID
    //#define clockIdt CLOCK_MONOTONIC // slow one
#else
    using TimePoint = std::chrono::high_resolution_clock::time_point;
#endif

class Timer
{
public:

    Timer();
    double getDuration();
    double getDuration() const;
    double getLapDuration();

    void continueTimer();
    void pauseTimer();
    void resetTimer();

    static TimePoint getTime();
    static double getTimeDifferenceInNanos(const TimePoint &fromTime, const TimePoint &toTime);

private:
    TimePoint startTime;
    TimePoint lastTime;
    double wholeDuration = 0.0;
    bool running = true;

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

