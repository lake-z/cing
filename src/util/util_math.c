#include "kernel_panic.h"
#include "util.h"

u64_t util_math_log_2_down(u64_t val)
{
  u64_t res;
  u64_t exp;

  kernel_assert(val > 0);

  res = 1;
  exp = 0;
  while (res < val) {
    res *= 2;
    exp++;
  }

  if (res > val) {
    exp--;
    res /= 2;
    kernel_assert(res < val);
  } else {
    kernel_assert(res == val);
  }
  return exp;
}

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

bo_t util_math_is_pow2(u64_t n)
{
  return base_likely(!((n) & ((n)-1)));
}
