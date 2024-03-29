/* Memory management subsystem. */

#include "containers_string.h"
#include "kernel_panic.h"
#include "log.h"
#include "mm_private.h"

/* @see
 * https://en.wikipedia.org/wiki/Executable_and_Linkable_Format#Section_header
 */
typedef struct multiboot_elf_sections_entry {
  uint32_t name;
  uint32_t type;
  uint64_t flags;
  uint64_t addr;
  uint64_t offset;
  uint64_t size;
  uint32_t link;
  uint32_t info;
  uint64_t alignment;
  uint64_t entry_size;
} base_struct_packed multiboot_elf_sections_entry_t;

typedef struct multiboot_tag_elf_sections {
  uint32_t num;
  uint32_t section_size;
  uint32_t shndx;
  multiboot_elf_sections_entry_t sections[];
} base_struct_packed multiboot_tag_elf_sections_t;

base_private uptr_t _kernel_start;
base_private uptr_t _kernel_end;

base_private inline bo_t _math_is_pow2(u64_t n)
{
  return base_likely(!((n) & ((n)-1)));
}

void mm_copy(byte_t *dest, const byte_t *src, usz_t copy_len)
{
  for (usz_t i = 0; i < copy_len; i++) {
    dest[i] = src[i];
  }
}

void mm_clean(vptr_t mem, usz_t size)
{
  kernel_assert(size > 0);
  for (usz_t i = 0; i < size; i++) {
    ((byte_t *)mem)[i] = 0;
  }
}

void mm_fill_bytes(byte_t *mem, usz_t size, byte_t data)
{
  for (usz_t i = 0; i < size; i++) {
    mem[i] = data;
  }
}

i64_t mm_compare(const byte_t *mem1, const byte_t *mem2, usz_t len)
{
  int cmp;
  kernel_assert(len > 0);
  for (usz_t i = 0; i < len; i++) {
    cmp = mem1[i] - mem2[i];
    if (cmp != 0) {
      break;
    }
  }
  return cmp;
}

uptr_t mm_align_up(uptr_t p, u64_t align)
{
  uptr_t intp = (uptr_t)p;
  u64_t a = align - 1;
  kernel_assert(align > 0);
  kernel_assert(_math_is_pow2(align));
  intp = ((intp + a) & ~a);
  return intp;
}

uptr_t mm_align_down(uptr_t p, u64_t align)
{
  uptr_t intp = (uptr_t)p;
  u64_t a = align - 1;
  kernel_assert(align > 0);
  kernel_assert(_math_is_pow2(align));
  intp = intp & ~a;
  return intp;
}

bo_t mm_align_check(uptr_t p, u64_t align)
{
  return ((uptr_t)p) % align == 0;
}

usz_t mm_align_class(uptr_t p)
{
  usz_t align = 1;
  while ((p % align) == 0) {
    align *= 2;
  }
  align /= 2;
  kernel_assert((p % align) == 0);
  kernel_assert(align > 1);
  return align;
}

base_private void _bootstrap_kernel_elf_symbols(
    const byte_t *kernel_elf_info, usz_t elf_info_len base_may_unuse)
{
  multiboot_tag_elf_sections_t *secs;
  multiboot_elf_sections_entry_t *entry;
  usz_t sec_cnt;
  usz_t sec_size;

  secs = (multiboot_tag_elf_sections_t *)kernel_elf_info;
  sec_cnt = secs->num;
  sec_size = secs->section_size;

  _kernel_start = U64_MAX;
  _kernel_end = 0;

  for (usz_t i = 0; i < sec_cnt; i++) {
    entry = (multiboot_elf_sections_entry_t *)(((uptr_t)secs->sections) +
                                               i * sec_size);
    if (entry->type != 0) {
      if (entry->addr < _kernel_start) {
        _kernel_start = entry->addr;
      }
      if ((entry->size + entry->addr) > _kernel_end) {
        _kernel_end = entry->size + entry->addr;
      }
    }
  }

  /* Kernel image is impossible to be that large */
  kernel_assert((_kernel_end - _kernel_start) < 1024 * 1024 * 1024);

  /* Verify this function must be with in kernel image */
  kernel_assert((uptr_t)mm_early_bootstrap < _kernel_end);
  kernel_assert((uptr_t)mm_early_bootstrap > _kernel_start);
  kernel_assert(mm_pa_range_valid(_kernel_start, _kernel_end));

  log_line_format(LOG_LEVEL_INFO, "kernel start: %lu, end: %lu", _kernel_start,
      _kernel_end);
}

void mm_early_bootstrap(const byte_t *kernel_elf_info,
    usz_t elf_info_len base_may_unuse,
    const byte_t *mmap_info,
    usz_t mmap_info_len)
{
  mm_frame_early_bootstrap(mmap_info, mmap_info_len);
  _bootstrap_kernel_elf_symbols(kernel_elf_info, elf_info_len);
  mm_page_early_bootstrap(_kernel_start, _kernel_end);
}

void mm_bootstrap(uptr_t boot_stack_bottom, uptr_t boot_stack_top)
{
  mm_frame_bootstrap();
  mm_page_bootstrap(
      _kernel_start, _kernel_end, boot_stack_bottom, boot_stack_top);
  mm_heap_bootstrap();
  mm_allocator_bootstrap();

  log_line_format(LOG_LEVEL_INFO,
      "mm initialize finished, %lu free frames available.",
      mm_frame_free_count());
}

uptr_t mm_va_pcie_cfg_space(void)
{
  return VA_48_PCIE_CFG_START;
}

uptr_t mm_va_pcie_cfg_space_bound(void)
{
  return VA_48_PCIE_CFG_END;
}

uptr_t mm_va_stack_bottom(void)
{
  return VA_48_STACK_BOTTOM;
}

uptr_t mm_va_stack_top(void)
{
  return VA_48_STACK_TOP;
}

#ifdef BUILD_SELF_TEST_ENABLED
void test_mm()
{
  test_heap();
  test_allocator();
}
#endif
