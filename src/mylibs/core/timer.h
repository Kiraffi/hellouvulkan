#pragma once

#include <time.h>
#include <stdint.h>

#if __unix__
    #define USE_UNIX_CLOCK_COARSE 1
#endif


class Timer
{
public:
// in linux the std::chrono::high_resolution_clock::now(); seems a lot slower call,
// than using coarse if doing a lot of calls. Although it might also give less accurate values.
#if USE_UNIX_CLOCK_COARSE
    // using timespec + CLOCK_MONOTONIC_COARSE
    using TimePoint = timespec;
    enum ClockType
    {
        //ClockId = CLOCK_MONOTONIC_COARSE, // spammable, but might be less inaccurate, 2-4ms resolution?
        //ClockId = CLOCK_REALTIME_COARSE,
        //ClockId = CLOCK_THREAD_CPUTIME_ID, //, not as slow but still slower, takes hit to spam this
        //ClockId = CLOCK_PROCESS_CPUTIME_ID,
        ClockId = CLOCK_MONOTONIC, // // slow one
        //ClockId = CLOCK_REALTIME, // // slow one
        ClockThreadId = CLOCK_THREAD_CPUTIME_ID,
        ClockCoarseId = CLOCK_MONOTONIC_COARSE,
    };
#else
    enum ClockType
    {
        ClockId = 1,
        ClockThreadId = 3,
        ClockCoarseId = 6,
    };
    using TimePoint = uint64_t;//std::chrono::high_resolution_clock::time_point;
#endif

    Timer();
    Timer(ClockType clockType);
    double getDuration();
    double getDuration() const;
    double getLapDuration();

    void continueTimer();
    void pauseTimer();
    void resetTimer();

    static TimePoint getTime(ClockType clockType);
    static double getTimeDifferenceInNanos(const TimePoint &fromTime, const TimePoint &toTime);

private:
    TimePoint startTime;
    TimePoint lastTime;
    // for linux
    ClockType clockType = ClockType::ClockId;
    double wholeDuration = 0.0;
    bool running = true;

};
const size_t foo = sizeof(Timer::TimePoint);

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

