/* Memory management subsystem. */
#include "kernel_panic.h"
#include "mem_private.h"
#include "util.h"

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

void mem_bootstrap_2(void)
{
  kernel_assert(boot_stage == MEM_BOOTSTRAP_STAGE_1);
  mem_page_bootstrap_2();
  boot_stage = MEM_BOOTSTRAP_STAGE_2;
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

uptr_t mem_align_up(uptr_t p, u64_t align)
{
  uptr_t intp = (uptr_t)p;
  u64_t a = align - 1;
  kernel_assert(align > 0);
  kernel_assert(util_math_is_pow2(align));
  intp = ((intp + a) & ~a);
  return intp;
}

uptr_t mem_align_down(uptr_t p, u64_t align)
{
  uptr_t intp = (uptr_t)p;
  u64_t a = align - 1;
  kernel_assert(align > 0);
  kernel_assert(util_math_is_pow2(align));
  intp = intp & ~a;
  return intp;
}
