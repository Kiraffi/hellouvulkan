#include "timer.h"

#include <stdio.h>


void setStartTime(Timer& timer)
{
	timer.startTime = std::chrono::high_resolution_clock::now();
}
void setEndTime(Timer& timer)
{
	timer.endTime = std::chrono::high_resolution_clock::now();
}
float getTime(const Timer& timer)
{
	return float((std::chrono::duration_cast<std::chrono::milliseconds>(timer.endTime - timer.startTime).count()) / 1000.0);
}

void printTime(const Timer& timer)
{
	float msTime = getTime(timer);
	printf("Timer: %fs\n", msTime);
}

void printNamedTimer(const Timer& timer, const char* name)
{
	float msTime = getTime(timer);
	printf("Timer %s: %f\n", name, msTime);
}
