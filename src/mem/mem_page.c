/* Memory virtual address space management. */
#include "cpu.h"
#include "mem_private.h"
#include "kernel_panic.h"

typedef enum {
  PAGE_SIZE_4K = PAGE_SIZE_VALUE_4K,
  PAGE_SIZE_2M = 2 * 1024 * 1024,
  PAGE_SIZE_1G = 1024 * 1024 * 1024,
} page_size_t;

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

typedef enum {
  TAB_LEV_4 = 4,
  TAB_LEV_3 = 3,
  TAB_LEV_2 = 2,
  TAB_LEV_1 = 1,
} tab_lev_t;
base_private const tab_lev_t _TAB_LEV_LOWEST = TAB_LEV_1;
base_private const tab_lev_t _TAB_LEV_HIGHEST = TAB_LEV_4;

#define _TAB_ENTRY_LEN 8
#define _TAB_ENTRY_COUNT 512

base_private tab_entry_t _tab_4[_TAB_ENTRY_COUNT] base_align(PAGE_SIZE_VALUE_4K);

base_private void _tab_load_root(tab_entry_t *p4)
{
  u64_t val;

  kernel_assert(mem_align_check((uptr_t)p4, PAGE_SIZE_4K));

  val = (uptr_t)p4;
  val = val & 0x000ffffffffff000;
  cpu_write_cr3(val);
}

base_private void _tab_zero(tab_entry_t *entry)
{
  mem_clean((byte_t *)entry, _TAB_ENTRY_LEN * _TAB_ENTRY_COUNT);
}

base_private bo_t _tab_entry_is_zero(tab_entry_t *entry)
{
  return (*(u64_t *)entry) == u64_literal(0);
}

/* Calculate table entry index of given virtual address and table level. */
base_private u16_t _tab_entry_idx(uptr_t va, tab_lev_t level)
{
  kernel_assert(level >= _TAB_LEV_LOWEST && level <= _TAB_LEV_HIGHEST);
  u64_t shift = (((u64_t)level - 1) * 9) + 12;
  u64_t idx = (va >> shift) & u64_literal(0x1ff);
  kernel_assert(idx < _TAB_ENTRY_COUNT);
  return (u16_t)idx;
}

base_private bo_t _tab_entry_present(tab_entry_t *entry)
{
  return entry->present;
}

/* Get the physical address of memory page, if this is the last level of page
 * table, or the next level og page table. */
base_private uptr_t _tab_entry_get_pa(tab_entry_t *ent)
{
  uptr_t result = *(uptr_t *)ent;
  result = result & 0x0ffffffffffff000;
  return result;
}

base_private void _tab_entry_init(tab_entry_t *e,
    tab_lev_t level,
    bo_t present,
    bo_t write,
    uptr_t padd,
    page_size_t padd_size)
{
  switch (level) {
  case TAB_LEV_4:
  case TAB_LEV_1:
    kernel_assert(padd_size == PAGE_SIZE_4K);
    break;
  case TAB_LEV_2:
    kernel_assert(padd_size == PAGE_SIZE_4K || padd_size == PAGE_SIZE_2M);
    break;
  case TAB_LEV_3:
    kernel_assert(padd_size == PAGE_SIZE_4K || padd_size == PAGE_SIZE_1G);
    break;
  default:
    kernel_panic("Invalid page table level");
  }

  kernel_assert(mem_align_check(padd, padd_size));
  kernel_assert(padd <= VA_48_LOW_END || padd >= VA_48_HIGH_START);

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

/* Mapping used during bootstrap stage 0 and 1. 
 * In this early stage, vitual addresses are always the same as physical 
 * addresses */
base_private bo_t _map_bootstrap_1(uptr_t va, uptr_t pa)
{
  tab_entry_t *tab;
  u16_t entry_idx;
  tab_entry_t *entry;
  bo_t ok;

  /* We use all 2MB pages in this stage */
  kernel_assert(mem_align_check((uptr_t)va, PAGE_SIZE_2M));
  kernel_assert(mem_align_check((uptr_t)pa, PAGE_SIZE_2M));

  tab = _tab_4;
  for (tab_lev_t lv = _TAB_LEV_HIGHEST; lv > TAB_LEV_2; lv--) {
    entry_idx = _tab_entry_idx(va, lv);
    entry = &(tab[entry_idx]);

    if (!_tab_entry_present(entry)) {
      byte_t *frame;

      kernel_assert(_tab_entry_is_zero(entry));

      frame = NULL;
      ok = mem_frame_alloc_bootstrap(&frame);
      if (ok) {
        kernel_assert(frame != NULL);
        _tab_entry_init(entry, lv, true, true, (uptr_t)frame, PAGE_SIZE_4K);
        _tab_zero((tab_entry_t *)frame);
      } else {
        break;
      }
    }
    tab = (tab_entry_t *)_tab_entry_get_pa(entry);
  }

  if (ok) {
    entry_idx = _tab_entry_idx(va, TAB_LEV_2);
    entry = &(tab[entry_idx]);
    kernel_assert(_tab_entry_is_zero(entry));
    _tab_entry_init(entry, TAB_LEV_2, true, true, (uptr_t)pa, PAGE_SIZE_2M);
  }

  return ok;
}

void mem_page_bootstrap_1(void)
{
  bo_t ok;
  uptr_t phy_0;
  uptr_t phy_z;
  uptr_t ker_0;
  uptr_t ker_z;
  uptr_t va;

  /* Supports to access as much as 1TB physical memory during bootstrap. */
  uptr_t pa_max = u64_literal(1024) * 1024 * 1024 * 1024;

  kernel_assert(boot_stage == MEM_BOOTSTRAP_STAGE_0);

  phy_0 = mem_pa_start();
  phy_z = mem_pa_end();
  ker_0 = mem_pa_ker_start();
  ker_z = mem_pa_ker_end();

  kernel_assert(phy_0 < ker_0);
  kernel_assert(ker_0 < ker_z);
  kernel_assert(ker_z < phy_z);
  kernel_assert(sizeof(tab_entry_t) == _TAB_ENTRY_LEN);

  _tab_zero(_tab_4);

  for (va = 0; (va + PAGE_SIZE_2M) < pa_max; va += PAGE_SIZE_2M) {
    ok = _map_bootstrap_1(va, va);
    kernel_assert(ok);
  }

  _tab_load_root(_tab_4);
}
