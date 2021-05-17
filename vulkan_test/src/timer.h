#pragma once

#include <chrono>

constexpr float billion = 1.0e9f;

struct Timer
{
	std::chrono::high_resolution_clock::time_point startTime;
	std::chrono::high_resolution_clock::time_point endTime;
};

void setStartTime(Timer& timer);
void setEndTime(Timer& timer);
float getTime(const Timer& timer);
void printTime(const Timer& timer);
void printNamedTimer(const Timer& timer, const char* name);

struct ScopedTimer
{
	ScopedTimer(const char *name)
	{
		setStartTime(timer);
		timerName = name;
	}

	ScopedTimer()
	{
		setStartTime(timer);
	}

	~ScopedTimer()
	{
		setEndTime(timer);
		if(timerName)
			printNamedTimer(timer, timerName);
		else
			printTime(timer);
	}

	Timer timer;
	const char *timerName = nullptr;
};

