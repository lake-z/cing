#include "mm_low.h"
#include "drivers_screen.h"
#include "kernel_panic.h"

base_private inline bo_t _math_is_pow2(u64_t n)
{
  return base_likely(!((n) & ((n) - 1)));
}

vptr_t mm_ptr_align_up(vptr_t p, u64_t align)
{
  uptr_t intp = (uptr_t)p;
  u64_t a = align - 1;
  kernel_assert(align > 0);
  kernel_assert(_math_is_pow2(align));
  intp = ((intp + a) & ~a);
  return (vptr_t)intp;
}
