/* Memory virtual address space management. */
#include "cpu.h"
#include "drivers_vesa.h"
#include "kernel_panic.h"
#include "log.h"
#include "mem_private.h"

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

base_private tab_entry_t _tab_4_bootstrap[_TAB_ENTRY_COUNT] base_align(
    PAGE_SIZE_VALUE_4K);

base_private tab_entry_t _tab_4[_TAB_ENTRY_COUNT] base_align(
    PAGE_SIZE_VALUE_4K);

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

/* Mapping used during bootstrap. */
base_private bo_t _map_bootstrap(tab_entry_t *root, uptr_t va, uptr_t pa)
{
  tab_entry_t *tab;
  u16_t entry_idx;
  tab_entry_t *entry;
  bo_t ok;

  /* We use all 2MB pages in this stage */
  kernel_assert(mem_align_check((uptr_t)va, PAGE_SIZE_2M));
  kernel_assert(mem_align_check((uptr_t)pa, PAGE_SIZE_2M));

  ok = true;
  tab = root;
  for (tab_lev_t lv = _TAB_LEV_HIGHEST; lv > TAB_LEV_2; lv--) {
    entry_idx = _tab_entry_idx(va, lv);
    entry = &(tab[entry_idx]);

    if (!_tab_entry_present(entry)) {
      byte_t *frame;

      kernel_assert(_tab_entry_is_zero(entry));

      frame = NULL;
      ok = mem_frame_alloc(&frame);
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

  _tab_zero(_tab_4_bootstrap);

  for (va = 0; (va + PAGE_SIZE_2M) < pa_max; va += PAGE_SIZE_2M) {
    ok = _map_bootstrap(_tab_4_bootstrap, va, va);
    kernel_assert(ok);
  }

  _tab_load_root(_tab_4_bootstrap);
}

base_private void _map_page(tab_entry_t *root, uptr_t va, uptr_t pa)
{
  tab_entry_t *tab;
  u16_t entry_idx;
  tab_entry_t *entry;

  kernel_assert(mem_align_check(va, PAGE_SIZE_4K));
  kernel_assert(mem_align_check(pa, PAGE_SIZE_4K));

  tab = root;
  for (tab_lev_t lv = _TAB_LEV_HIGHEST; lv > TAB_LEV_1; lv--) {
    entry_idx = _tab_entry_idx(va, lv);
    entry = &(tab[entry_idx]);

    if (!_tab_entry_present(entry)) {
      bo_t ok;
      byte_t *frame;

      kernel_assert(_tab_entry_is_zero(entry));

      frame = NULL;
      ok = mem_frame_alloc(&frame);
      kernel_assert(ok);
      kernel_assert(frame != NULL);

      _tab_entry_init(entry, lv, true, true, (uptr_t)frame, PAGE_SIZE_4K);
      _tab_zero((tab_entry_t *)frame);
    }

    /* Iterate to next level of page table. */
    tab = (tab_entry_t *)_tab_entry_get_pa(entry);
  }

  entry_idx = _tab_entry_idx(va, TAB_LEV_1);
  entry = &(tab[entry_idx]);
  kernel_assert(_tab_entry_is_zero(entry));
  _tab_entry_init(entry, TAB_LEV_1, true, true, (uptr_t)pa, PAGE_SIZE_4K);
}

base_private void _map_impl(tab_entry_t *root, /*Root of page table hierachy */
    uptr_t va,    /* Start of virtual address to be mapped */
    ucnt_t n_pg,  /* Page count of virtual address to be mapped */
    pa_list_t *pa /* Physical address to be mapped */
)
{
  u64_t pa_idx;
  u64_t pa_page;
  kernel_assert(mem_align_check(va, PAGE_SIZE_4K));
  kernel_assert_d(n_pg == pa_list_n_page(pa));

  pa_idx = 0;
  pa_page = 0;
  for (u64_t i = 0; i < n_pg;) {
    kernel_assert_d(pa_idx < pa_list_n_range(pa));
    if (pa_page < pa_list_range_n_page(pa, pa_idx)) {
      uptr_t pg_va = va + i * PAGE_SIZE_4K;
      uptr_t pg_pa = pa_list_range_pa(pa, pa_idx) + pa_page * PAGE_SIZE_4K;
      _map_page(root, pg_va, pg_pa);
      i++;
      pa_page++;
    } else {
      pa_idx++;
      pa_page = 0;
    }
  }
}

void mem_page_map(uptr_t va, /* Start of virtual address to be mapped */
    ucnt_t n_pg,             /* Page count of virtual address to be mapped */
    pa_list_t *pa            /* Physical address to be mapped */
)
{
  kernel_assert_d(boot_stage > MEM_BOOTSTRAP_STAGE_0);
  _map_impl(_tab_4, va, n_pg, pa);
}

void mem_page_bootstrap_2(void)
{
  uptr_t ker_0_va;
  uptr_t ker_z_va;
  u64_t ker_n_page;
  uptr_t fb;
  u64_t fb_len;
  pa_list_t *pa;

  _tab_zero(_tab_4);

  /* Map since address 0, since otherwise we may aceess not aviabole address. */
  ker_0_va = 0;
  kernel_assert(mem_align_check(ker_0_va, PAGE_SIZE_4K));

  ker_z_va = mem_pa_ker_end();
  kernel_assert(mem_align_check(ker_0_va, PAGE_SIZE_4K));

  ker_n_page = (ker_z_va - ker_0_va) / PAGE_SIZE_4K;

  pa = pa_list_new_bootstrap(1);

  /* Mapping kernel binary, a virtual to physical address direct mapping. */
  pa_list_set_range(pa, 0, ker_0_va, ker_n_page);
  _map_impl(_tab_4, ker_0_va, ker_n_page, pa);

  /* Mapping VESA frame buffer. */
  fb = (uptr_t)d_vesa_get_frame_buffer();
  kernel_assert(mem_align_check(fb, PAGE_SIZE_4K));
  fb_len = d_vesa_get_frame_buffer_len();
  kernel_assert((fb_len % PAGE_SIZE_4K) == 0);
  kernel_assert(fb_len < (VA_48_FRAME_BUFFER_END - VA_48_FRAME_BUFFER));
  pa_list_set_range(pa, 0, fb, fb_len / PAGE_SIZE_4K);
  d_vesa_set_frame_buffer((byte_t *)VA_48_FRAME_BUFFER);
  _map_impl(_tab_4, VA_48_FRAME_BUFFER, fb_len / PAGE_SIZE_4K, pa);

  pa_list_free_bootstrap(pa);
  pa = NULL;

  _tab_load_root(_tab_4);
}
