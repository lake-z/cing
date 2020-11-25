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

#define _TAB_ENTRY_LEN 8
#define _TAB_ENTRY_COUNT 512
base_private tab_entry_t _tab_4[_TAB_ENTRY_COUNT] base_align(_PAGE_SIZE_4K);

base_private uptr_t _early_map_end;

base_private vptr_t _direct_vadd;
/* Table entries to establish the direct mapping,
 * level 3..1 table entries here  */
base_private
    byte_t _direct_tab_space[_TAB_ENTRY_LEN * _TAB_ENTRY_COUNT * 3] base_align(
        _PAGE_SIZE_4K);
base_private tab_entry_t *_direct_tab[3];
base_private tab_entry_t *_direct_tab_entry;

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

/* Forwarded declaration of functions */
base_private tab_entry_index_t _vadd_tab_index(uptr_t va, tab_level_t level);
uptr_t _direct_access_get_padd(vptr_t va);
base_private usz_t _vadd_page_offset(uptr_t va, tab_level_t lv);

base_private void _tlb_flush(vptr_t va)
{
  asm volatile("invlpg (%0)" ::"r"(va) : "memory");
}

base_private void _tab_entry_zero(tab_entry_t *entry)
{
  *(u64_t *)entry = 0;
}

base_private bo_t _tab_entry_is_zero(tab_entry_t *entry)
{
  return (*(u64_t *)entry) == 0;
}

base_private bo_t _tab_entry_is_present(tab_entry_t *entry)
{
  return entry->present;
}

base_private bo_t _tab_entry_is_huge(tab_entry_t *entry)
{
  return entry->huge;
}

base_private uptr_t _tab_entry_get_padd(tab_entry_t *ent)
{
  uptr_t result = *(uptr_t *)ent;
  result = result & 0x0ffffffffffff000;
  return result;
}

base_private void _tab_entry_init(tab_entry_t *e,
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
  kernel_assert(padd <= VADD_48_LOW_END || padd >= VADD_48_HIGH_START);

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

base_private void _tab_zero(tab_entry_t *entry)
{
  mm_clean(entry, sizeof(_TAB_ENTRY_LEN * _TAB_ENTRY_COUNT));
}

base_private bo_t _tab_is_zero(tab_entry_t *entry)
{
  bo_t is_zero;

  kernel_assert(mm_align_check((uptr_t)entry, PAGE_SIZE_4K));

  is_zero = true;
  for (usz_t i = 0; i < _TAB_ENTRY_COUNT; i++) {
    is_zero = _tab_entry_is_zero(&entry[i]);
    if (!is_zero) {
      break;
    }
  }

  return is_zero;
}

base_private void _tab_root_load(tab_entry_t *p4)
{
  u64_t val;

  kernel_assert(mm_align_check((uptr_t)p4, PAGE_SIZE_4K));

  val = (uptr_t)p4;
  val = val & 0x000ffffffffff000;
  cpu_write_cr3(val);
}

base_private bo_t _map_early(uptr_t va, uptr_t pa)
{
  /* In this early stage, vitual addresses are always the same as physical 
   * addresses */
  tab_entry_t *tab;
  tab_entry_index_t entry_idx;
  tab_entry_t *entry;
  bo_t ok;

  /* We use all 2MB pages in early stage */
  kernel_assert(mm_align_check((uptr_t)va, PAGE_SIZE_2M));
  kernel_assert(mm_align_check((uptr_t)pa, PAGE_SIZE_2M));

  tab = _tab_4;
  for (tab_level_t lv = TAB_LEVEL_4; lv > TAB_LEVEL_2; lv--) {
    entry_idx = _vadd_tab_index(va, lv);
    entry = &(tab[entry_idx]);

    if (!_tab_entry_is_present(entry)) {
      byte_t *frame;

      kernel_assert(_tab_entry_is_zero(entry));

      frame = NULL;
      ok = mm_frame_get_early(&frame);
      if (ok) {
        kernel_assert(frame != NULL);
        _tab_entry_init(entry, lv, true, true, (uptr_t)frame, PAGE_SIZE_4K);
        _tab_zero((tab_entry_t *)frame);
      } else {
        break;
      }
    }
    tab = (tab_entry_t *)_tab_entry_get_padd(entry);
  }

  if (ok) {
    entry_idx = _vadd_tab_index(va, TAB_LEVEL_2);
    entry = &(tab[entry_idx]);
    kernel_assert(_tab_entry_is_zero(entry));
    _tab_entry_init(entry, TAB_LEVEL_2, true, true, (uptr_t)pa, PAGE_SIZE_2M);
  }

  return ok;
}

/*
base_private bo_t _map(vptr_t va, vptr_t pa)
{
  uptr_t tab_pa;
  tab_entry_t *tab_va;
  tab_entry_index_t entry_idx;
  tab_entry_t *entry;
  tab_level_t lv;
  bo_t ok;

   We only supports 4K sized pages 
  kernel_assert(mm_align_check((uptr_t)va, PAGE_SIZE_4K));
  kernel_assert(mm_align_check((uptr_t)pa, PAGE_SIZE_4K));

  / Physical address is the same with virtual address /
  tab_pa = (uptr_t)_tab_4;
  for (lv = TAB_LEVEL_4; lv > TAB_LEVEL_1; lv--) {
    tab_va = mm_page_direct_access_setup(tab_pa);
    entry_idx = _vadd_tab_index(va, lv);
    entry = &(tab_va[entry_idx]);
    mm_page_direct_access_reset();
    tab_va = NULL;

    if (!_tab_entry_is_present(entry)) {
      uptr_t frame_pa;
      byte_t *frame_va;

      kernel_assert(_tab_entry_is_zero(entry));

      frame_pa = 0;
      ok = mm_frame_get(&frame_pa);
      if (ok) {
        kernel_assert(frame_pa != 0);
        _tab_entry_init(entry, lv, true, true, frame_pa, PAGE_SIZE_4K);

        frame_va = mm_page_direct_access_setup(frame_pa);
        _tab_zero((tab_entry_t *)frame_va);
        mm_page_direct_access_reset();
        frame_va = NULL;
      } else {
        break;
      }
    }

    tab_pa = _tab_entry_get_padd(entry);
  }

  if (ok) {
    tab_va = mm_page_direct_access_setup(tab_pa);
    entry_idx = _vadd_tab_index(va, TAB_LEVEL_1);
    entry = &(tab_va[entry_idx]);
    kernel_assert(_tab_entry_is_zero(entry));
    _tab_entry_init(entry, lv, true, true, (uptr_t)pa, PAGE_SIZE_4K);
    mm_page_direct_access_reset();
  }

  return ok;
}
*/

base_private uptr_t _unmap(uptr_t va, page_size_t size)
{
  uptr_t tab_pa[5];
  tab_entry_t *tab_va_tmp;
  tab_entry_t *entry_va_tmp;
  tab_entry_index_t entry_idx;
  tab_level_t lv;
  uptr_t pa;
  bo_t huge;
  bo_t tab_empty;

  kernel_assert(mm_align_check(va, size));

  _tlb_flush((vptr_t)va);

  /* Traverse page tables find out page table tree nodes pointing to the given 
   * virtual address. */
  tab_pa[TAB_LEVEL_4] = (uptr_t)_tab_4;
  for (lv = TAB_LEVEL_4; lv >= TAB_LEVEL_1;) {
    entry_idx = _vadd_tab_index(va, lv);

    /* Set up a direct access map, read out needed entry fields and reset the
     * direct map immediately. */
    tab_va_tmp = mm_page_direct_access_setup(tab_pa[lv]);
    entry_va_tmp = &(tab_va_tmp[entry_idx]);

    kernel_assert(_tab_entry_is_present(entry_va_tmp));
    huge = _tab_entry_is_huge(entry_va_tmp);
    if (huge) {
      kernel_assert(lv == TAB_LEVEL_2);
    }

    lv--;

    /* Read out physical address of next level page table */
    tab_pa[lv] = _tab_entry_get_padd(entry_va_tmp);

    /* tab_va_tmp and entry_va_tmp will be invalid right after here */
    tab_va_tmp = NULL;
    entry_va_tmp = NULL;
    mm_page_direct_access_reset();

    if (huge) {
      break;
    }
  }

  /* Here lv equals TAB_LEVEL_1 for 2M pages or 0 for 4K pages. */
  lv++;
  kernel_assert(lv == TAB_LEVEL_1 || lv == TAB_LEVEL_2);

  pa = tab_pa[lv - 1] + _vadd_page_offset(va, lv);

  if (base_likely(lv == TAB_LEVEL_1)) {
    kernel_assert(size == PAGE_SIZE_4K);
    kernel_assert(huge == false);
    kernel_assert(mm_align_check(pa, PAGE_SIZE_4K));
    mm_frame_return((vptr_t)va, pa);
  } else if (lv == TAB_LEVEL_2) {
    uptr_t pa_freed;
    uptr_t va_freed;

    kernel_assert(size == PAGE_SIZE_2M);
    kernel_assert(huge == true);
    kernel_assert(mm_align_check(pa, PAGE_SIZE_2M));

    /* Cut a 2M page used in early stage into 512 4K pages, and unmap them one
     * by one. */
    va_freed = va;
    for (pa_freed = pa; pa_freed < pa + PAGE_SIZE_2M;
         pa_freed += PAGE_SIZE_4K) {
      mm_frame_return((vptr_t)va_freed, pa_freed);
      va_freed += PAGE_SIZE_4K;
    }
  } else {
    /* We use 2M and 4K pages only */
    kernel_panic("Invalid page table level");
  }

  /* Traverse page table from down up, free table frame if it is all zero. */
  tab_empty = true;
  for (; tab_empty && (lv <= TAB_LEVEL_4); lv++) {
    entry_idx = _vadd_tab_index(va, lv);
    tab_va_tmp = mm_page_direct_access_setup(tab_pa[lv]);
    entry_va_tmp = &(tab_va_tmp[entry_idx]);
    _tab_entry_zero(entry_va_tmp);
    tab_empty = _tab_is_zero(tab_va_tmp);
    if (tab_empty) {
      mm_frame_return((vptr_t)tab_va_tmp, tab_pa[lv]);
    }

    tab_va_tmp = NULL;
    entry_va_tmp = NULL;
    mm_page_direct_access_reset();
  }

  return pa;
}

base_private tab_entry_index_t _vadd_tab_index(uptr_t va, tab_level_t level)
{
  kernel_assert(level >= _TAB_LEVEL_LOWEST && level <= _TAB_LEVEL_HIGHEST);
  u64_t shift = (((u64_t)level - 1) * 9) + 12;
  u64_t idx = (va >> shift) & u64_literal(0x1ff);
  kernel_assert(idx < _TAB_ENTRY_COUNT);
  return (tab_entry_index_t)idx;
}

base_private usz_t _vadd_page_offset(uptr_t va, tab_level_t lv)
{
  if (base_likely(lv == TAB_LEVEL_1)) {
    return va & 0x0000000000000fff;
  } else if (lv == TAB_LEVEL_2) {
    return va & 0x00000000001fffff;
  } else {
    /* Currently supports 4K and 2M sized pages only */
    kernel_panic("Invalid page table level");
  }
}

bo_t vadd_get_padd(vptr_t va, uptr_t *out_pa)
{
  uptr_t tab_pa;
  tab_entry_t *tab_va;
  tab_level_t lv;
  bo_t present;

  tab_pa = (uptr_t)_tab_4;
  present = true;
  for (lv = TAB_LEVEL_4; lv >= TAB_LEVEL_1; lv--) {
    tab_va = mm_page_direct_access_setup(tab_pa);
    tab_entry_index_t enidx = _vadd_tab_index((uptr_t)va, lv);
    tab_entry_t *en = &tab_va[enidx];
    mm_page_direct_access_reset();

    if (!_tab_entry_is_present(en)) {
      kernel_assert(_tab_entry_is_zero(en));
      present = false;
      break;
    } else {
      kernel_assert(!_tab_entry_is_huge(en));
      tab_pa = _tab_entry_get_padd(en);
    }
  }

  if (present) {
    (*out_pa) = (tab_pa + _vadd_page_offset((uptr_t)va, lv));
  }

  return present;
}

base_private void _init_direct_access(void)
{
  tab_entry_t *tab;
  tab_entry_index_t en_idx;
  tab_entry_t *en;

  _direct_vadd = (vptr_t)VADD_48_HIGH_START;
  kernel_assert(mm_align_check((uptr_t)_direct_tab, PAGE_SIZE_4K));

  for (usz_t i = 0; i < 3; i++) {
    _direct_tab[i] = (tab_entry_t *)_direct_tab_space + i * PAGE_SIZE_4K;
    kernel_assert(mm_align_check((uptr_t)_direct_tab[i], PAGE_SIZE_4K));
  }

  tab = _tab_4;
  for (tab_level_t lv = TAB_LEVEL_4; lv > TAB_LEVEL_1; lv--) {
    usz_t arr_idx;

    en_idx = _vadd_tab_index((uptr_t)_direct_vadd, lv);
    en = &tab[en_idx];

    kernel_assert(!_tab_entry_is_present(en));
    kernel_assert(_tab_entry_is_zero(en));

    arr_idx = lv - 1 - 1;
    tab = _direct_tab[arr_idx];

    _tab_entry_init(en, lv, true, true, (uptr_t)tab, PAGE_SIZE_4K);
    _tab_zero(tab);
  }

  en_idx = _vadd_tab_index((uptr_t)_direct_vadd, TAB_LEVEL_1);
  _direct_tab_entry = _direct_tab[TAB_LEVEL_1 - 1];
  _tab_entry_zero(_direct_tab_entry);
}

vptr_t mm_page_direct_access_setup(uptr_t padd)
{
  kernel_assert(mm_align_check(padd, PAGE_SIZE_4K));
  kernel_assert(_tab_entry_is_zero(_direct_tab_entry));
  _tab_entry_init(
      _direct_tab_entry, TAB_LEVEL_1, true, true, padd, PAGE_SIZE_4K);
  return _direct_vadd;
}

void mm_page_direct_access_reset(void)
{
  kernel_assert(_tab_entry_is_present(_direct_tab_entry));
  _tab_entry_zero(_direct_tab_entry);
  _tlb_flush(_direct_vadd);
}

uptr_t _direct_access_get_padd(vptr_t va)
{
  kernel_assert(_tab_entry_is_present(_direct_tab_entry));
  kernel_assert(va == _direct_vadd);
  return _tab_entry_get_padd(_direct_tab_entry);
}

base_private byte_t _test_direct_access_bytes[PAGE_SIZE_4K] base_align(
    _PAGE_SIZE_4K);

base_private void _test_direct_access(void)
{
  log_line_start(LOG_LEVEL_DEBUG);
  log_str(LOG_LEVEL_DEBUG, __FUNCTION__);
  log_str(LOG_LEVEL_DEBUG, " started..");
  log_line_end(LOG_LEVEL_DEBUG);

  for (usz_t i = 0; i < PAGE_SIZE_4K; i++) {
    byte_t *va = mm_page_direct_access_setup((uptr_t)_test_direct_access_bytes);
    _test_direct_access_bytes[i] = 0xFE;
    kernel_assert(va[i] == 0xFE);
    mm_page_direct_access_reset();
  }

  log_line_start(LOG_LEVEL_DEBUG);
  log_str(LOG_LEVEL_DEBUG, "Pass.");
  log_line_end(LOG_LEVEL_DEBUG);
}

void mm_page_early_init(uptr_t kernel_start, uptr_t kernel_end)
{
  uptr_t va;
  bo_t ok;
  uptr_t phy_start = padd_start();
  uptr_t phy_end = padd_end();

  kernel_assert(sizeof(tab_entry_t) == _TAB_ENTRY_LEN);
  kernel_assert(phy_start < kernel_start);
  kernel_assert(phy_end > kernel_end);
  kernel_assert(kernel_start < kernel_end);

  _tab_zero(_tab_4);

  for (va = 0; (va + PAGE_SIZE_2M) < phy_end; va += PAGE_SIZE_2M) {
    ok = _map_early(va, va);
    if (!ok)
      break;
  }

  _early_map_end = va;

  _tab_root_load(_tab_4);
}

void mm_page_init(uptr_t kernel_start, uptr_t kernel_end)
{
  uptr_t unmap;
  uptr_t phy_end = padd_end();

  kernel_assert(kernel_start < kernel_end);

  _init_direct_access();

#ifdef BUILD_TEST_MODE_ENABLED
  _test_direct_access();
#endif

  unmap = mm_align_up(kernel_end + 1, PAGE_SIZE_2M);
  for (; (unmap + PAGE_SIZE_2M) < phy_end; unmap += PAGE_SIZE_2M) {
    _unmap(unmap, PAGE_SIZE_2M);
  }

  kernel_assert(unmap == _early_map_end);
}
