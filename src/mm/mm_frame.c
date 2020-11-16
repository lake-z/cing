#include "kernel_panic.h"
#include "log.h"
#include "mm_private.h"

typedef struct {
  vptr_t base;
  usz_t len;
} phy_mem_section_t;

/* Max number of physical memory sections allowed */
#define _PHY_MEM_SECTION_MAX 1024
base_private phy_mem_section_t _phy_mem_secs[_PHY_MEM_SECTION_MAX];
base_private usz_t _phy_mem_sec_count;

/* Total size of physical memory */
base_private usz_t _phy_mem_size;

base_private void _init_mmap_info(const byte_t *ptr, usz_t size)
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

  _phy_mem_sec_count = 0;
  _phy_mem_size = 0;
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

    /* Ignores memory below 1MB */
    if (type == 1 && (base >= 1024 * 1024)) {
      kernel_assert(_phy_mem_sec_count < _PHY_MEM_SECTION_MAX);
      _phy_mem_secs[_phy_mem_sec_count].base = (vptr_t)base;
      _phy_mem_secs[_phy_mem_sec_count].len = len;
      _phy_mem_sec_count++;
      _phy_mem_size += len;
    }
  }

  log_line_start(LOG_LEVEL_INFO);
  log_str(LOG_LEVEL_INFO, "Physical memory size: ");
  log_uint_of_size(LOG_LEVEL_INFO, _phy_mem_size);
  log_line_end(LOG_LEVEL_INFO);
}

void mm_frame_init(const byte_t *mmap_info, usz_t mmap_info_len)
{
  _init_mmap_info(mmap_info, mmap_info_len);
}

bo_t mm_phy_addr_range_valid(uptr_t start, uptr_t end)
{
  bo_t found = false;
  for (usz_t i = 0; i < _phy_mem_sec_count; i++) {
    uptr_t sec_start = (uptr_t)_phy_mem_secs[i].base;
    uptr_t sec_end = sec_start + _phy_mem_secs[i].len;

    if (sec_start <= start && sec_end > end) {
      found = true;
      break;
    }
  }
  return found;
}
