#ifndef ___MM
#define ___MM

#include "base.h"

void mm_init(const byte_t *kernel_elf_info,
    usz_t elf_info_len base_may_unuse,
    const byte_t *mmap_info,
    usz_t mmap_info_len);
vptr_t mm_align_up(vptr_t p, u64_t align) base_no_null;

#endif
