#include "cpu.h"
#include "kernel_panic.h"
#include "log.h"
#include "mm_private.h"

#define _PAGE_SIZE_4K 4096

typedef enum {
  PAGE_SIZE_4K = _PAGE_SIZE_4K,
  PAGE_SIZE_2M = 2 * 1024 * 1024,
  PAGE_SIZE_1G = 1024 * 1024 * 1024,
} page_size_t;

typedef u64_t page_no_t;
typedef u16_t tab_entry_index_t;

typedef enum {
  TAB_LEVEL_4 = 4,
  TAB_LEVEL_3 = 3,
  TAB_LEVEL_2 = 2,
  TAB_LEVEL_1 = 1,
} tab_level_t;
base_private const tab_level_t _TAB_LEVEL_LOWEST = TAB_LEVEL_1;
base_private const tab_level_t _TAB_LEVEL_HIGHEST = TAB_LEVEL_4;

typedef struct {
  bo_t present : 1;
  bo_t writable : 1;
  bo_t user_accessible : 1;
  bo_t write_through : 1;
  bo_t no_cache : 1;
  bo_t accessed : 1;
  bo_t dirty : 1;
  bo_t huge : 1;
  bo_t global : 1;
  byte_t os_use_1 : 3;
  u64_t phy_addr : 40;
  u16_t os_use_2 : 11;
  bo_t no_exe : 1;
} base_struct_packed tab_entry_t;

base_private uptr_t _early_end_page;

base_private const u64_t _TAB_ENTRY_LEN = 8;
#define _TAB_ENTRY_COUNT 512
base_private tab_entry_t _tab_4[_TAB_ENTRY_COUNT] base_align(_PAGE_SIZE_4K);

base_private vptr_t _sec_access_vadd;
base_private tab_entry_t _sec_access_tab_3[_TAB_ENTRY_COUNT] base_align(_PAGE_SIZE_4K);
base_private tab_entry_t _sec_access_tab_2[_TAB_ENTRY_COUNT] base_align(_PAGE_SIZE_4K);
base_private tab_entry_t _sec_access_tab_1[_TAB_ENTRY_COUNT] base_align(_PAGE_SIZE_4K);
base_private tab_entry_t *_sec_access_tab_entry;

/* Virtual address layout
 * --------
 * [*] kernel_start .. kernel_end is a direct mapping
 * [*] First page after VADD_48_HIGH_START is a temp mapping to handle cases
 *     such as double page fault.
 */
base_private const uptr_t VADD_LOW_START = u64_literal(0);
base_private const uptr_t VADD_48_LOW_END = u64_literal(0x00007fffffffffff);
base_private const uptr_t VADD_48_HIGH_START = u64_literal(0xffff800000000000);
base_private const uptr_t VADD_HIGH_END = u64_literal(0xffffffffffffffff);

base_private tab_entry_index_t _vadd_tab_index(vptr_t va, tab_level_t level)
{
  kernel_assert(level >= _TAB_LEVEL_LOWEST && level <= _TAB_LEVEL_HIGHEST);
  u64_t shift = (((u64_t)level - 1) * 9) + 12;
  u64_t idx = (((uptr_t)va) >> shift) & u64_literal(0x1ff);
  kernel_assert(idx < _TAB_ENTRY_COUNT);
  return (tab_entry_index_t)idx;
}

base_private void _tab_zero(tab_entry_t *entry)
{
  mm_clean(entry, sizeof(_TAB_ENTRY_LEN * _TAB_ENTRY_COUNT));
}

base_private void _tab_root_load(tab_entry_t *p4)
{
  u64_t val;

  kernel_assert(mm_align_check((uptr_t)p4, PAGE_SIZE_4K));

  val = (uptr_t)p4;
  val = val & 0x000ffffffffff000;
  cpu_write_cr3(val);
}

base_private bo_t _tab_entry_is_zero(tab_entry_t *entry)
{
  return (*(u64_t *)entry) == 0;
}

base_private bo_t _tab_entry_is_present(tab_entry_t *entry)
{
  return entry->present;
}

base_private tab_entry_t *_tab_entry_get_padd(tab_entry_t *ent)
{
  uptr_t result = *(uptr_t *)ent;
  result = result & 0x0ffffffffffff000;
  return (tab_entry_t *)result;
}

base_private void _tab_entry_init(
    tab_entry_t *e,
    tab_level_t level,
    bo_t present,
    bo_t write,
    uptr_t padd,
    page_size_t padd_size)
{
  switch (level) {
  case TAB_LEVEL_4:
  case TAB_LEVEL_1:
    kernel_assert(padd_size == PAGE_SIZE_4K);
    break;
  case TAB_LEVEL_2:
    kernel_assert(padd_size == PAGE_SIZE_4K || padd_size == PAGE_SIZE_2M);
    break;
  case TAB_LEVEL_3:
    kernel_assert(padd_size == PAGE_SIZE_4K || padd_size == PAGE_SIZE_1G);
    break;
  default:
    kernel_panic("Invalid page table level");
  }

  kernel_assert(mm_align_check(padd, padd_size));
  kernel_assert(padd < VADD_48_LOW_END);

  (*(u64_t *)e) = padd;
  if (present) {
    e->present = 1;
  }
  if (write) {
    e->writable = 1;
  }
  if (padd_size == PAGE_SIZE_2M || padd_size == PAGE_SIZE_1G) {
    e->huge = 1;
  }
}

base_private bo_t _map(vptr_t va, vptr_t pa, page_size_t size)
{
  tab_entry_t *tab;
  tab_entry_index_t entry_idx;
  tab_entry_t *entry;
  byte_t *frame;
  tab_level_t lv;
  tab_level_t lowest_lv;
  bo_t ok;

  if (size == PAGE_SIZE_4K) {
    lowest_lv = TAB_LEVEL_1;
  } else if (size == PAGE_SIZE_2M) {
    lowest_lv = TAB_LEVEL_2;
  } else {
     /* We currently only supports 2M page size in early stage, and 4K sized 
      * pages after early stage. */
     kernel_panic("Invalid memory page size");
  }
  kernel_assert(mm_align_check((uptr_t)va, size));
  kernel_assert(mm_align_check((uptr_t)pa, size));

  tab = _tab_4;
  for (lv = TAB_LEVEL_4; lv > lowest_lv; lv--) {
    entry_idx = _vadd_tab_index(va, lv);
    entry = &(tab[entry_idx]);

    if (!_tab_entry_is_present(entry)) {
      kernel_assert(_tab_entry_is_zero(entry));

      frame = NULL;
      ok = mm_frame_get(&frame);
      if (ok) {
        kernel_assert(frame != NULL);
        _tab_zero((tab_entry_t *)frame);
        _tab_entry_init(entry, lv, true, true, (uptr_t)frame, PAGE_SIZE_4K);
      } else {
        break;
      }
    }

    tab = _tab_entry_get_padd(entry);
  }

  if (ok) {
    if (size == PAGE_SIZE_4K) {
      kernel_assert(lv == TAB_LEVEL_1);
    } else if (PAGE_SIZE_2M) {
      kernel_assert(lv == TAB_LEVEL_2);
    } else {
      kernel_panic("Invalid memory page size");
    }

    entry_idx = _vadd_tab_index(va, lv);
    entry = &(tab[entry_idx]);
    _tab_entry_init(entry, lv, true, true, (uptr_t)pa, size);
  }

  return ok;
}

void mm_page_early_init(
    uptr_t kernel_start, uptr_t kernel_end, uptr_t phy_start, uptr_t phy_end)
{
  uptr_t map_start = mm_align_down(kernel_start, PAGE_SIZE_2M);
  uptr_t map_end = phy_end;
  uptr_t va;
  bo_t ok;

  kernel_assert(sizeof(tab_entry_t) == _TAB_ENTRY_LEN);
  kernel_assert(phy_start <= kernel_start);

  kernel_assert(map_start <= kernel_start);
  kernel_assert(map_end > kernel_end);
  kernel_assert(mm_align_check(map_start, PAGE_SIZE_2M));

  _tab_zero(_tab_4);

  for (va = map_start; (va + PAGE_SIZE_2M) < map_end; va += PAGE_SIZE_2M) {
    ok = _map((vptr_t)va, (vptr_t)va, PAGE_SIZE_2M);
    if (!ok)
      break;
  }
  _early_end_page = va;

  _tab_root_load(_tab_4);
}

base_private void _init_sec_access_path(void)
{
  tab_entry_index_t en_idx;
  tab_entry_t *en;

  _sec_access_vadd = (vptr_t)VADD_48_HIGH_START;

  en_idx = _vadd_tab_index(_sec_access_vadd, TAB_LEVEL_4);
  en = &_tab_4[en_idx];
  kernel_assert(!_tab_entry_is_present(en));
  kernel_assert(_tab_entry_is_zero(en));
  _tab_entry_init(en, TAB_LEVEL_4, true, true, (uptr_t)_sec_access_tab_3, PAGE_SIZE_4K);

  _tab_zero(_sec_access_tab_3);
  en_idx = _vadd_tab_index(_sec_access_vadd, TAB_LEVEL_3);
  en = &_sec_access_tab_3[en_idx];
  _tab_entry_init(en, TAB_LEVEL_3, true, true, (uptr_t)_sec_access_tab_2, PAGE_SIZE_4K);

  _tab_zero(_sec_access_tab_2);
  en_idx = _vadd_tab_index(_sec_access_vadd, TAB_LEVEL_2);
  en = &_sec_access_tab_2[en_idx];
  _tab_entry_init(en, TAB_LEVEL_2, true, true, (uptr_t)_sec_access_tab_1, PAGE_SIZE_4K);

  _tab_zero(_sec_access_tab_1);
  en_idx = _vadd_tab_index(_sec_access_vadd, TAB_LEVEL_1);
  _sec_access_tab_entry = &_sec_access_tab_1[en_idx];
}

void mm_page_sec_access_setup(vptr_t padd)
{
  kernel_assert(mm_align_check((uptr_t)padd, PAGE_SIZE_4K));
  _tab_entry_init(_sec_access_tab_entry, TAB_LEVEL_1, true, true, (uptr_t)padd, PAGE_SIZE_4K);
}

void mm_page_init(uptr_t kernel_start, uptr_t kernel_end)
{
  _init_sec_access_path();
  (void)(kernel_start + kernel_end);
}
