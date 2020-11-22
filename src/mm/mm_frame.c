#include "kernel_panic.h"
#include "log.h"
#include "mm_private.h"

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
  struct frame_free *next;
} frame_free_t;

/* Max number of physical memory sections allowed */
#define _PHY_MEM_SECTION_MAX 1024
base_private phy_mem_section_t _phy_mem_secs[_PHY_MEM_SECTION_MAX];
base_private usz_t _phy_mem_sec_count;

/* Total size of physical memory */
base_private usz_t _phy_mem_size;

#define _EARLY_FRAME_COUNT 16 * 1024
byte_t _early_frame_pool[_EARLY_FRAME_COUNT * FRAME_SIZE_4K] base_align(4096);
base_private usz_t _early_frame_count;
base_private bo_t _is_early_stage;

base_private byte_t *_frames_start;

base_private frame_free_t *_free_head;
base_private u64_t _free_count;

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

void mm_frame_init(u64_t kernel_end)
{
  uptr_t frame;

  kernel_assert(_is_early_stage);

  _frames_start = (byte_t *)mm_align_up(kernel_end, FRAME_SIZE_4K);

  _free_head = NULL;
  _free_count = 0;

  /* Last 2MB space is not mapped by the early stage frame manager */
  for (frame = (uptr_t)_frames_start; (frame + FRAME_SIZE_2M) < padd_end();
       frame += FRAME_SIZE_4K) {
    mm_frame_return((byte_t *)frame);
  }

  _is_early_stage = false;

  log_line_start(LOG_LEVEL_INFO);
  log_str(LOG_LEVEL_INFO, "mm frames manager inited, ");
  log_uint(LOG_LEVEL_INFO, _free_count);
  log_str(LOG_LEVEL_INFO, " free frames available.");
  log_line_end(LOG_LEVEL_INFO);
}

base_private base_must_check bo_t _get_early(byte_t **out_frame)
{
  bo_t ok;
  if (_early_frame_count < _EARLY_FRAME_COUNT) {
    (*out_frame) = _early_frame_pool + _early_frame_count * FRAME_SIZE_4K;
    _early_frame_count++;
    ok = true;
  } else {
    ok = false;
  }
  return ok;
}

bo_t mm_frame_get(byte_t **out_frame)
{
  if (base_unlikely(_is_early_stage)) {
    return _get_early(out_frame);
  } else {
    bo_t ok;

    if (_free_head == NULL) {
      ok = false;
    } else {
      kernel_assert(mm_align_check((uptr_t)_free_head, FRAME_SIZE_4K));

      /* Free frame may not be vitual address accessable, set up a safe mapping
       * from virtual address to physical address */
      mm_page_sec_access_setup(_free_head);

      (*out_frame) = (byte_t *)_free_head;
      _free_head = _free_head->next;
      _free_count--;
      ok = true;
    }
    return ok;
  }
}

void mm_frame_return(byte_t *frame)
{
  frame_free_t *free;

  kernel_assert(frame != NULL);
  free = (frame_free_t *)frame;
  free->next = _free_head;
  _free_head = (frame_free_t *)frame;
  _free_count++;
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
