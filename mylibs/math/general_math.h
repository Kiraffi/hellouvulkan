#pragma once

#include <cmath>

constexpr double Pi = 3.14159265358979323846264; //constant expression

float toRadians(float angle)
{
	return float(180.0 / Pi * angle);
}

template<class T>
T clamp(T v, T a, T b)
{
	v = v < b ? v : b;
	v = v > a ? v : a;
	return v;
}