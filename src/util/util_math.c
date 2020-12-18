#include "kernel_panic.h"
#include "util.h"

u64_t util_math_log_2_up(u64_t val)
{
  u64_t res;

  if (val == 0) {
    return 1;
  }

  res = 0;
  val--;
  while (true) {
    val /= 2;
    if (val == 0) {
      break;
    }
    res++;
  }
  return res + 1;
}

u64_t util_math_2_exp(u8_t exp)
{
  return u64_literal(1) << exp;
}
