#ifndef ___KERNEL_PANIC
#define ___KERNEL_PANIC

#include "base.h"

#define kernel_assert(expr) \
  do { \
    if(base_unlikely(!(expr))) {\
      _kernel_assert_fail(#expr, __FILE__, __LINE__); \
    } \
  } while(0) \

base_no_return _kernel_assert_fail(const char *expr, const char *file, usz_t line);

#endif
