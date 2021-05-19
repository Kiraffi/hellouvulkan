#include "timer.h"

#include <stdio.h>


Timer::Timer()
{
	startTime = std::chrono::high_resolution_clock::now();
	lastTime = startTime;
}

double Timer::getDuration() const
{
	std::chrono::high_resolution_clock::time_point newTime = std::chrono::high_resolution_clock::now();

	double dur = std::chrono::duration_cast<std::chrono::nanoseconds>
		(newTime - startTime).count() * 1.0e-9;

	return dur;
}

double Timer::getLapDuration()
{
	std::chrono::high_resolution_clock::time_point newTime = std::chrono::high_resolution_clock::now();

	double dur = std::chrono::duration_cast<std::chrono::nanoseconds>
		(newTime - lastTime).count() * 1.0e-9;

	lastTime = newTime;
	return dur;
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
