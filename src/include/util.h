#ifndef ___UTIL
#define ___UTIL

#include "base.h"

/* @returns The smallest x which satisfy 2^x >= val*/
u64_t util_math_log_2_up(u64_t val);

/* @returns The biggest x which satisfy 2^x <= val */
u64_t util_math_log_2_down(u64_t val);

/* @returns 2^exp */
u64_t util_math_2_exp(u8_t exp);

bo_t util_math_is_pow2(u64_t n);

u64_t util_rand_int_next(u64_t curr);

#endif
