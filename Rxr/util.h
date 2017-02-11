#ifndef lenzhound_util_h
#define lenzhound_util_h
#include "constants.h"

const int BIT_SHIFT = 15;

inline long abs32(long a) {
  return (a < 0) ? -a : a;
}

inline long max32(long a, long b) {
  return (a > b) ? a : b;
}

inline long min32(long a, long b) {
  return (a < b) ? a : b;
}

inline long clamp32(long x, long min, long max) {
	return min32(max, max32(min, x));
}

inline long i16_to_fixed(int a) {
  return ((long)a) << BIT_SHIFT;
}

inline long i32_to_fixed(long a) {
  return a << BIT_SHIFT;
}

inline int fixed_to_i16(long a) {
  return (int)(a >> BIT_SHIFT);
}

inline long fixed_to_i32(long a) {
  return a >> BIT_SHIFT;
}

inline long fixed_mult(long a, long b) {
  return (a * b) >> BIT_SHIFT;
}

inline long fixed_div(long a, long b) {
  return (a << BIT_SHIFT) / b;
}

const long FIXED_ONE = i16_to_fixed(1);

#endif // lenzhound_util_h
