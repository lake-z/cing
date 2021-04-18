/* Public header of memory management subsystem. */
#ifndef ___MEM
#define ___MEM

#include "base.h"

typedef struct pa_list pa_list_t;

void mem_bootstrap_1(
    const byte_t *mb_elf,  /* Multiboot kernel elf sections info */
    usz_t mb_elf_len,      /* Length of @mb_elf*/
    const byte_t *mb_mmap, /* Multiboot memory map info */
    usz_t mb_mmap_len      /* Length of @mb_mmap */
);
void mem_bootstrap_2(void);
void mem_bootstrap_3(void);

void mem_page_map(uptr_t va, /* Start of virtual address to be mapped */
    ucnt_t n_pg,             /* Page count of virtual address to be mapped */
    pa_list_t *pa            /* Physical address to be mapped */
);

void mem_clean(byte_t *mem, usz_t size);
bo_t mem_align_check(uptr_t p, u64_t align);
uptr_t mem_align_up(uptr_t p, u64_t align);
uptr_t mem_align_down(uptr_t p, u64_t align);

#endif
