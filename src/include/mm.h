#ifndef ___MM
#define ___MM

#include "base.h"

uptr_t mm_align_up(uptr_t p, u64_t align);
uptr_t mm_align_down(uptr_t p, u64_t align);
bo_t mm_align_check(uptr_t p, u64_t align);
void mm_copy(byte_t *dest, const byte_t *src, usz_t copy_len);
void mm_clean(vptr_t mem, usz_t size);
i64_t mm_compare(byte_t *mem1, byte_t *mem2, usz_t len);

void mm_early_init(const byte_t *kernel_elf_info,
    usz_t elf_info_len base_may_unuse,
    const byte_t *mmap_info,
    usz_t mmap_info_len);

#endif
