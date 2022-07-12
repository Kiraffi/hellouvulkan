#pragma once

template <typename T>
void isPodType();

template <typename T>
void isNotPodType();

template <typename T>
void newInPlace(T *ptr, const T &value);

