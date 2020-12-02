#ifndef ___KERNEL_PANIC
#define ___KERNEL_PANIC

#include "base.h"

#define kernel_assert(expr)                                                    \
  do {                                                                         \
    if (base_unlikely(!(expr))) {                                              \
      _kernel_assert_fail(#expr, __FILE__, __LINE__);                          \
    }                                                                          \
  } while (0)

#ifdef BUILD_DEBUG_ENABLED
#define kernel_assert_d(expr) kernel_assert(expr)
#else
#define kernel_assert_d(expr) expr
#endif

base_no_return _kernel_assert_fail(
    const char *expr, const char *file, usz_t line);

#define kernel_panic(msg)                                                      \
  do {                                                                         \
    _kernel_panic(__FILE__, __LINE__, msg);                                    \
  } while (0)

base_no_return _kernel_panic(const char *file, usz_t line, const char *msg);

/* Stack smashing protecter interface, used by Compiler -fstack-protector-* 
   only, nerver used directly. */
void __stack_chk_fail(void);

#endif
