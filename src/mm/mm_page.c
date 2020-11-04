#include "kernel_panic.h"
#include "mm_private.h"

typedef struct {
  vptr_t base;
  usz_t len;
} segment_physical_t;

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
} base_struct_packed page_tab_entry;

#define _SEGMENT_PHYSICAL_SLOT_COUNT 1024
base_private usz_t _phy_segs_count;
base_private segment_physical_t _phy_segs[_SEGMENT_PHYSICAL_SLOT_COUNT];

void mm_page_init_mmap_info(const byte_t *ptr, usz_t size)
{
  u32_t entry_size;
  u32_t entry_ver;
  u64_t entry_cnt;

  entry_size = *(u32_t *)ptr;
  ptr += 4;

  entry_ver = *(u32_t *)ptr;
  ptr += 4;

  kernel_assert(entry_ver == 0);
  kernel_assert(entry_size == 24);
  kernel_assert((size - 8) % entry_size == 0);
  entry_cnt = (size - 8) / entry_size;

  _phy_segs_count = 0;
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

    if (type == 1) {
      kernel_assert(_phy_segs_count < _SEGMENT_PHYSICAL_SLOT_COUNT);
      _phy_segs[_phy_segs_count].base = (vptr_t)base;
      _phy_segs[_phy_segs_count].len = len;
      _phy_segs_count++;
    }
  }
}

bo_t mm_page_phy_addr_range_valid(uptr_t start, uptr_t end)
{
  bo_t found = false;
  for (usz_t i = 0; i < _phy_segs_count; i++) {
    uptr_t seg_start = (uptr_t)_phy_segs[i].base;
    uptr_t seg_end = seg_start + _phy_segs[i].len;

    if (seg_start <= start && seg_end > end) {
      found = true;
      break;
    }
  }
  return found;
}
