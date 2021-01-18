#ifndef ___MM
#define ___MM

#include "base.h"

typedef struct mm_allocator mm_allocator_t;

uptr_t mm_vadd_stack_bp_bottom_get(void);
uptr_t mm_vadd_stack_bp_top_get(void);

uptr_t mm_align_up(uptr_t p, u64_t align);
uptr_t mm_align_down(uptr_t p, u64_t align);
bo_t mm_align_check(uptr_t p, u64_t align);
void mm_copy(byte_t *dest, const byte_t *src, usz_t copy_len);
void mm_clean(vptr_t mem, usz_t size);
void mm_fill_bytes(byte_t *mem, usz_t size, byte_t data);
i64_t mm_compare(const byte_t *mem1, const byte_t *mem2, usz_t len);

void mm_early_bootstrap(const byte_t *kernel_elf_info,
    usz_t elf_info_len base_may_unuse,
    const byte_t *mmap_info,
    usz_t mmap_info_len);

void mm_bootstrap(uptr_t boot_stack_bottom, uptr_t boot_stack_top);

mm_allocator_t *mm_allocator_new(void);
vptr_t mm_allocate(mm_allocator_t *all, usz_t size, usz_t align);
void mm_allocator_free(mm_allocator_t *all);

#ifdef BUILD_BUILTIN_TEST_ENABLED
/* Built-in tests declarations */
void test_mm(void);
#endif

#endif
