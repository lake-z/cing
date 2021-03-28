/* Public header of memory management subsystem. */
#ifndef ___MEM
#define ___MEM

#include "base.h"

void mem_bootstrap_1(
    const byte_t *mb_elf,  /* Multiboot kernel elf sections info */
    usz_t mb_elf_len,      /* Length of @mb_elf*/
    const byte_t *mb_mmap, /* Multiboot memory map info */
    usz_t mb_mmap_len      /* Length of @mb_mmap */
);

void mem_clean(byte_t *mem, usz_t size);
bo_t mem_align_check(uptr_t p, u64_t align);

#endif
