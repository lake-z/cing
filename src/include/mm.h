#ifndef ___MM
#define ___MM

#include "base.h"

void mm_init(const byte_t *boot_info, usz_t info_len);
vptr_t mm_align_up(vptr_t p, u64_t align) base_no_null;

#endif
