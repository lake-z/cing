#include "kernel_panic.h"
#include "log.h"
#include "mm_private.h"

/* Describes a section of available physical memory. */
typedef struct {
  uptr_t base;
  usz_t len;
} phy_mem_section_t;

typedef enum {
  FRAME_SIZE_4K = 4 * 1024,
  FRAME_SIZE_2M = 2 * 1024 * 1024,
  FRAME_SIZE_1G = 1024 * 1024 * 1024,
} frame_size_t;

typedef struct frame_free {
  uptr_t next; /* Physical address of next free frame */
} frame_free_t;

/* Max number of physical memory sections allowed */
#define _PHY_MEM_SECTION_MAX 1024
/* All available physical memory sections are saved in this array. */
base_private phy_mem_section_t _phy_mem_secs[_PHY_MEM_SECTION_MAX];
/* Element count of array @_phy_mem_secs */
base_private usz_t _phy_mem_sec_count;

/* Total size of physical memory */
base_private usz_t _phy_mem_size;

/* Frames can be used during early stage of mm bootstrap. */
#define _EARLY_FRAME_CAP 16 * 1024
byte_t _early_frame_pool[_EARLY_FRAME_CAP * FRAME_SIZE_4K] base_align(4096);
base_private usz_t _early_frame_count;
base_private bo_t _is_early_stage;

/* Physical address of the head of free frame list.
 * Every 4K frame can be cast to frame_free_t and linked with next pointer. */
base_private uptr_t _free_head;
base_private ucnt_t _free_count;

/* Initialize avaliable physical memory sections according to Multiboot memory 
 * map. */
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
    if (type == 1) {
      kernel_assert(_phy_mem_sec_count < _PHY_MEM_SECTION_MAX);
      _phy_mem_secs[_phy_mem_sec_count].base = (uptr_t)base;
      _phy_mem_secs[_phy_mem_sec_count].len = len;
      if (_phy_mem_sec_count > 0) {
        uptr_t last_end = _phy_mem_secs[_phy_mem_sec_count - 1].base;
        last_end += _phy_mem_secs[_phy_mem_sec_count - 1].len;
        kernel_assert(_phy_mem_secs[_phy_mem_sec_count].base > last_end);
      }
      _phy_mem_sec_count++;
      _phy_mem_size += len;
    }
  }

  log_line_start(LOG_LEVEL_INFO);
  log_str(LOG_LEVEL_INFO, "Physical memory size: ");
  log_uint_of_size(LOG_LEVEL_INFO, _phy_mem_size);
  log_line_end(LOG_LEVEL_INFO);
}

void mm_frame_early_init(const byte_t *mmap_info, usz_t mmap_info_len)
{
  _early_frame_count = 0;
  _init_mmap_info(mmap_info, mmap_info_len);
  _is_early_stage = true;
}

void mm_frame_init(void)
{
  kernel_assert(_is_early_stage);

  _free_head = UPTR_NULL;
  _free_count = 0;
  _is_early_stage = false;
}

base_must_check bo_t mm_frame_alloc_early(byte_t **out_frame)
{
  bo_t ok;

  kernel_assert(_is_early_stage);

  if (_early_frame_count < _EARLY_FRAME_CAP) {
    (*out_frame) = _early_frame_pool + _early_frame_count * FRAME_SIZE_4K;
    _early_frame_count++;
    ok = true;
  } else {
    ok = false;
  }
  return ok;
}

base_must_check bo_t mm_frame_alloc(uptr_t *out_frame)
{
  bo_t ok;
  frame_free_t *free_head_va;

  kernel_assert(!_is_early_stage);

  if (_free_head == 0) {
    ok = false;
  } else {
    kernel_assert(mm_align_check(_free_head, FRAME_SIZE_4K));
    (*out_frame) = _free_head;

    /* Free frame may not be vitual address accessable, set up a safe mapping
     * from virtual address to physical address */
    free_head_va = mm_page_direct_access_setup(*out_frame);
    _free_head = free_head_va->next;
    _free_count--;
    mm_page_direct_access_reset();
    ok = true;
  }
  return ok;
}

void mm_frame_free(byte_t *frame_va, uptr_t frame_pa)
{
  frame_free_t *free;

  free = (frame_free_t *)frame_va;
  free->next = _free_head;

  _free_head = frame_pa;
  _free_count++;
}

ucnt_t mm_frame_free_count(void)
{
  return _free_count;
}

uptr_t padd_start(void)
{
  kernel_assert(_phy_mem_sec_count > 0);
  return _phy_mem_secs[0].base;
}

uptr_t padd_end(void)
{
  kernel_assert(_phy_mem_sec_count > 0);
  uptr_t mem = _phy_mem_secs[_phy_mem_sec_count - 1].base;
  mem += _phy_mem_secs[_phy_mem_sec_count - 1].len;
  return mem;
}

bo_t padd_range_valid(uptr_t start, uptr_t end)
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
