#pragma once

//#include <cmath>
constexpr double Pi = 3.14159265358979323846264; //constant expression

static inline float toRadians(float angle)
{
	return float(180.0f / pii * angle);
}

template<class T>
static inline T clamp(T v, T a, T b)
{
	v = v < b ? v : b;
	v = v > a ? v : a;
	return v;
}