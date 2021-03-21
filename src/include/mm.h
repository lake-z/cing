#ifndef ___MM
#define ___MM

#include "base.h"

typedef struct mm_allocator mm_allocator_t;

uptr_t mm_va_stack_bottom(void);
uptr_t mm_va_stack_top(void);
uptr_t mm_va_pcie_cfg_space(void);
uptr_t mm_va_pcie_cfg_space_bound(void);

uptr_t mm_align_up(uptr_t p, u64_t align);
uptr_t mm_align_down(uptr_t p, u64_t align);
bo_t mm_align_check(uptr_t p, u64_t align);
/* Given address @p, calculate the max class it is aligned to. */
usz_t mm_align_class(uptr_t p);

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

bo_t mm_page_map(uptr_t va, uptr_t pa);

#ifdef BUILD_SELF_TEST_ENABLED
/* Built-in tests declarations */
void test_mm(void);
#endif

#endif
