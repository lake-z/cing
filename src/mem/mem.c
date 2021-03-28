/* Memory management subsystem. */
#include "kernel_panic.h"
#include "mem_private.h"

mem_bootstrap_stage_t boot_stage = MEM_BOOTSTRAP_STAGE_0;

void mem_bootstrap_1(const byte_t *mb_elf,
    usz_t mb_elf_len,
    const byte_t *mb_mmap,
    usz_t mb_mmap_len)
{
  kernel_assert(boot_stage == MEM_BOOTSTRAP_STAGE_0);
  mem_frame_bootstrap_1(mb_elf, mb_elf_len, mb_mmap, mb_mmap_len);
  mem_page_bootstrap_1();
  boot_stage = MEM_BOOTSTRAP_STAGE_1;
}

void mem_clean(byte_t *mem, usz_t size)
{
  kernel_assert(size > 0);
  for (usz_t i = 0; i < size; i++) {
    (mem)[i] = (byte_t)0;
  }
}

bo_t mem_align_check(uptr_t p, u64_t align)
{
  return ((uptr_t)p) % align == 0;
}
