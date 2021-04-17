/* Physical memory management. */

#include "drivers_pcie.h"
#include "drivers_vesa.h"
#include "kernel_panic.h"
#include "log.h"
#include "mem_private.h"

/* Describes a section of available physical memory. */
typedef struct {
  uptr_t base;
  usz_t len;
} section_t;

/*
 * Multiboot ELF sections, Reference:
 * https://en.wikipedia.org/wiki/Executable_and_Linkable_Format#Section_header
 */
typedef struct mb_elf_sec_entry {
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
} base_struct_packed mb_elf_sec_entry_t;

/* Multiboot ELF sections tag. */
typedef struct mb_tag_elf_secs {
  uint32_t num;
  uint32_t section_size;
  uint32_t shndx;
  mb_elf_sec_entry_t sections[];
} base_struct_packed mb_tag_elf_secs_t;

/* A list of physical address ranges. */
struct pa_list {
  ucnt_t n;     /* Address range count */
  uptr_t *pa;   /* @n Start physical addresses */
  ucnt_t *n_pg; /* Page count of physical address ranges */
};

/* Max number of physical memory sections allowed */
#define _SECTION_CAP 32
/* All available physical memory sections are saved in this array.
 * Sections are guranteed to be orderer in memory address and never overlap. */
base_private section_t _sections[_SECTION_CAP];
/* Array size of @_sections */
base_private usz_t _sec_cnt = 0;

/* Total size of physical memory */
base_private usz_t _pmem_size;

base_private uptr_t _kern_start_pa;
base_private uptr_t _kern_end_pa;

/* Frames can be used during stage 1 of mem subsystem bootstrap. */
#define _FRAME_CAP_BOOTSTRAP 16 * 1024
byte_t _frame_pool_bootstrap[_FRAME_CAP_BOOTSTRAP * PAGE_SIZE_4K] base_align(
    PAGE_SIZE_VALUE_4K);
/* Frames used in early stage. */
base_private usz_t _frame_count_bootstrap;

#define _PA_LIST_CAP_BOOTSTRAP 1024
base_private byte_t _pa_list_bootstrap_pool[_PA_LIST_CAP_BOOTSTRAP];
base_private byte_t *_pa_list_bootstrap_next;

base_private void _bootstrap_mmap_info(const byte_t *ptr, usz_t size)
{
  u32_t entry_size;
  u32_t entry_ver;
  u64_t entry_cnt;

  kernel_assert(boot_stage == MEM_BOOTSTRAP_STAGE_0);

  entry_size = *(u32_t *)ptr;
  ptr += 4;

  entry_ver = *(u32_t *)ptr;
  ptr += 4;

  kernel_assert(entry_ver == 0);
  kernel_assert(entry_size == 24);
  kernel_assert((size - 8) % entry_size == 0);
  entry_cnt = (size - 8) / entry_size;

  _sec_cnt = 0;
  _pmem_size = 0;
  for (u64_t en = 0; en < entry_cnt; en++) {
    u64_t base;
    u64_t len;
    u32_t type;
    u32_t reserve;

    base = *(u64_t *)ptr;
    ptr += 8;
    len = *(u64_t *)ptr;
    ptr += 8;
    type = *(u32_t *)ptr;
    ptr += 4;
    reserve = *(u32_t *)ptr;
    base_mark_unuse(reserve); /* Do not guranteed to be 0 on real hardware */
    ptr += 4;

    if (type != 1)
      continue;

    kernel_assert(_sec_cnt < _SECTION_CAP);
    _sections[_sec_cnt].base = (uptr_t)base;
    _sections[_sec_cnt].len = len;

    if (_sec_cnt > 0) {
      uptr_t last_end = _sections[_sec_cnt - 1].base;
      last_end += _sections[_sec_cnt - 1].len;
      kernel_assert(_sections[_sec_cnt].base > last_end);
    }

    _sec_cnt++;
    _pmem_size += len;
  }

  log_line_format(LOG_LEVEL_INFO, "Physical memory size: %lu", _pmem_size);
}

base_private void _bootstrap_kernel_elf_symbols(
    const byte_t *elf_info, usz_t elf_info_len base_may_unuse)
{
  mb_tag_elf_secs_t *secs;
  mb_elf_sec_entry_t *entry;
  usz_t sec_cnt;
  usz_t sec_size;

  kernel_assert(boot_stage == MEM_BOOTSTRAP_STAGE_0);

  secs = (mb_tag_elf_secs_t *)elf_info;
  sec_cnt = secs->num;
  sec_size = secs->section_size;

  _kern_start_pa = U64_MAX;
  _kern_end_pa = 0;

  for (usz_t i = 0; i < sec_cnt; i++) {
    entry = (mb_elf_sec_entry_t *)(((uptr_t)secs->sections) + i * sec_size);
    if (entry->type != 0) {
      if (entry->addr < _kern_start_pa) {
        _kern_start_pa = entry->addr;
      }
      if ((entry->size + entry->addr) > _kern_end_pa) {
        _kern_end_pa = entry->size + entry->addr;
      }
    }
  }

  /* Kernel image is impossible to be that large */
  kernel_assert((_kern_end_pa - _kern_start_pa) < 1024 * 1024 * 1024);

  /* Verify this function must be with in kernel image */
  kernel_assert((uptr_t)_bootstrap_kernel_elf_symbols < _kern_end_pa);
  kernel_assert((uptr_t)_bootstrap_kernel_elf_symbols > _kern_start_pa);
  kernel_assert(mem_pa_range_valid(_kern_start_pa, _kern_end_pa));

  log_line_format(LOG_LEVEL_INFO, "kernel start: %lu, end: %lu", _kern_start_pa,
      _kern_end_pa);
}

void mem_frame_bootstrap_1(const byte_t *mb_elf,
    usz_t mb_elf_len,
    const byte_t *mb_mmap,
    usz_t mb_mmap_len)
{
  kernel_assert(boot_stage == MEM_BOOTSTRAP_STAGE_0);

  _frame_count_bootstrap = 0;
  _pa_list_bootstrap_next = _pa_list_bootstrap_pool;

  _bootstrap_mmap_info(mb_mmap, mb_mmap_len);
  _bootstrap_kernel_elf_symbols(mb_elf, mb_elf_len);
}

base_must_check bo_t mem_frame_alloc(byte_t **out_frame)
{
  bo_t ok;

  kernel_assert(boot_stage < MEM_BOOTSTRAP_STAGE_FINISH);

  if (_frame_count_bootstrap < _FRAME_CAP_BOOTSTRAP) {
    (*out_frame) =
        _frame_pool_bootstrap + _frame_count_bootstrap * PAGE_SIZE_4K;
    _frame_count_bootstrap++;
    ok = true;
  } else {
    ok = false;
  }
  return ok;
}

/*
base_private mem_heap_t *_heaps[_SECTION_CAP];
base_private usz_t _heap_cnt;


base_private bo_t _pa_overlaps_pcie_cfg_space(uptr_t pa, usz_t len)
{
  ucnt_t cnt = d_pcie_group_get_cnt();
  bo_t ret = false;
  for (ucnt_t i = 0; i < cnt; i++) {
    uptr_t cfg_start = d_pcie_group_get_cfg_pa(i);

    ret = pa_range_overlaps(pa, len, cfg_start, d_pcie_group_get_cfg_len());
    if (ret)
      break;
  }
  return ret;
}

void mem_frame_bootstrap_2(void)
{
  uptr_t fb = (uptr_t)d_vesa_get_frame_buffer();
  usz_t fb_len = d_vesa_get_frame_buffer_len();

  _heap_cnt = 0;
  for (usz_t i = 0; i < _sec_cnt; i++) {
    uptr_t heap_start;
    uptr_t heap_end;
    bo_t used = pa_range_overlaps(_sections[i].base, _sections[i].len,
        _kern_start_pa, _kern_end_pa - _kern_start_pa);
    if (used)
      continue;

    used = pa_range_overlaps(_sections[i].base, _sections[i].len, fb, fb_len);
    if (used)
      continue;

    used = _pa_overlaps_pcie_cfg_space(_sections[i].base, _sections[i].len);
    if (used)
      continue;

    heap_start = _sections[i].base;
    heap_end = heap_start + _sections[i].len;
    _heaps[_heap_cnt++] = mem_heap_new_bootstrap(heap_start, heap_end);
    log_line_format(LOG_LEVEL_INFO, "Availabe physical memory heap %lu: %lu",
        _heap_cnt - 1, mem_heap_size(_heaps[_heap_cnt - 1]));
  }
}
*/

uptr_t mem_pa_start(void)
{
  kernel_assert_d(_sec_cnt > 0);
  return _sections[0].base;
}

uptr_t mem_pa_end(void)
{
  uptr_t mem;

  kernel_assert_d(_sec_cnt > 0);
  mem = _sections[_sec_cnt - 1].base;
  mem += _sections[_sec_cnt - 1].len;
  return mem;
}

bo_t pa_range_overlaps(uptr_t a1, usz_t len1, uptr_t a2, usz_t len2)
{
  bo_t result = ((a1 + len1) < a2) || ((a2 + len2) < a1);
  return !result;
}

bo_t mem_pa_range_valid(uptr_t start, uptr_t end)
{
  bo_t found = false;
  kernel_assert_d(_sec_cnt > 0);
  for (usz_t i = 0; i < _sec_cnt; i++) {
    uptr_t sec_start = (uptr_t)_sections[i].base;
    uptr_t sec_end = sec_start + _sections[i].len;

    if (sec_start <= start && sec_end > end) {
      found = true;
      break;
    }
  }
  return found;
}

uptr_t mem_pa_ker_start(void)
{
  return _kern_start_pa;
}

uptr_t mem_pa_ker_end(void)
{
  return _kern_end_pa;
}

pa_list_t *pa_list_new_bootstrap(u64_t n_range)
{
  pa_list_t *res;
  byte_t *end = _pa_list_bootstrap_pool + _PA_LIST_CAP_BOOTSTRAP;

  kernel_assert(boot_stage < MEM_BOOTSTRAP_STAGE_FINISH);
  kernel_assert(_pa_list_bootstrap_next < end);

  res = (pa_list_t *)_pa_list_bootstrap_next;
  _pa_list_bootstrap_next += sizeof(pa_list_t);
  kernel_assert(_pa_list_bootstrap_next <= end);

  res->n = n_range;

  res->pa = (uptr_t *)_pa_list_bootstrap_next;
  _pa_list_bootstrap_next += sizeof(uptr_t) * n_range;
  kernel_assert(_pa_list_bootstrap_next <= end);

  res->n_pg = (u64_t *)_pa_list_bootstrap_next;
  _pa_list_bootstrap_next += sizeof(u64_t) * n_range;
  kernel_assert(_pa_list_bootstrap_next <= end);

  return res;
}

void pa_list_free_bootstrap(pa_list_t *list)
{
  /* Do nothing. */
  base_mark_unuse(list);
}

void pa_list_set_range(pa_list_t *list, u64_t range, uptr_t pa, u64_t n_pg)
{
  kernel_assert_d(range < list->n);
  kernel_assert_d(mem_align_check(pa, PAGE_SIZE_4K));

  list->pa[range] = pa;
  list->n_pg[range] = n_pg;
}

ucnt_t pa_list_n_page(pa_list_t *list)
{
  ucnt_t n_page = 0;
  for (ucnt_t i = 0; i < list->n; i++) {
    n_page += list->n_pg[i];
  }
  return n_page;
}

u64_t pa_list_n_range(pa_list_t *list)
{
  kernel_assert_d(list->n > 0);
  return list->n;
}

u64_t pa_list_range_n_page(pa_list_t *list, u64_t range_idx)
{
  kernel_assert_d(range_idx < list->n);
  return list->n_pg[range_idx];
}

uptr_t pa_list_range_pa(pa_list_t *list, u64_t range_idx)
{
  kernel_assert_d(range_idx < list->n);
  kernel_assert_d(mem_align_check(list->pa[range_idx], PAGE_SIZE_4K));
  return list->pa[range_idx];
}
