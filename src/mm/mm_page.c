#include "cpu.h"
#include "log.h"
#include "kernel_panic.h"
#include "mm_private.h"
#include "drivers_screen.h"
#include "containers_string.h"

typedef u16_t tab_entry_index_t;

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
} base_struct_packed page_tab_entry_t;

#define _SEGMENT_PHYSICAL_SLOT_COUNT 1024
base_private usz_t _phy_segs_count;
base_private segment_physical_t _phy_segs[_SEGMENT_PHYSICAL_SLOT_COUNT];
base_private usz_t _phy_mem_total;

base_private const page_tab_level_t PAGE_TAB_LEVEL_LOWEST = 1;
base_private const page_tab_level_t PAGE_TAB_LEVEL_HIGHEST = 4;
base_private const u64_t PAGE_TAB_ENTRY_COUNT = 512;

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

  _phy_mem_total = 0;
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
      _phy_mem_total += len;
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

base_private tab_entry_index_t _vaddr_tab_index(vptr_t va, page_tab_level_t level)
{
  kernel_assert(level >= PAGE_TAB_LEVEL_LOWEST && level <= PAGE_TAB_LEVEL_HIGHEST);
  u64_t shift = (((u64_t)level - 1) * 9) + 12;
  u64_t idx = (((uptr_t)va) >> shift) & u64_literal(0x1ff);
  kernel_assert(idx < PAGE_TAB_ENTRY_COUNT);
  return (u16_t)idx;
}

base_private bo_t _page_tab_entry_is_inited(page_tab_entry_t *page_tab, tab_entry_index_t idx)
{
  return (*(u64_t *)(page_tab + idx)) != 0;
}

base_private vptr_t _entry_get_paddr(page_tab_entry_t *ent)
{
  uptr_t result = *(uptr_t *)ent;
  result = result & 0x0ffffffffffff000;
  //FIXME: handler higher half of vaddr space
  return (vptr_t)result;
}

base_private void _early_map_add(page_tab_entry_t *p4, uptr_t *next_frame,
    vptr_t vaddr, vptr_t paddr) {
  base_private const usz_t _MSG_CAP = 128;
  ch_t msg[_MSG_CAP];
  usz_t msg_len;
  const ch_t *msg_part;

  tab_entry_index_t idx;
  page_tab_entry_t *ptab;
  page_tab_entry_t *entry;

  kernel_assert(mm_align_check(vaddr, PAGE_SIZE_2M));
  kernel_assert(mm_align_check(paddr, PAGE_SIZE_2M));

  ptab = p4;
  for(page_tab_level_t i = PAGE_TAB_LEVEL_HIGHEST; i > 2; i--) {
    idx = _vaddr_tab_index(vaddr, i);
    kernel_assert(idx < PAGE_TAB_ENTRY_COUNT);
    entry = ptab + idx;


    if(!_page_tab_entry_is_inited(ptab, idx)) {
      mm_clean((vptr_t)(*next_frame), PAGE_SIZE_4K);
      *(uptr_t *)entry = *next_frame;
     (*next_frame) += PAGE_SIZE_4K;
      entry->present = 1;
      entry->writable = 1;
     }
    ptab = (page_tab_entry_t *)_entry_get_paddr(entry);
  }

  idx = _vaddr_tab_index(vaddr, 2);

  kernel_assert(idx < PAGE_TAB_ENTRY_COUNT);
  entry = ptab + idx;

  msg_len = 0;
  msg_part = "Mapping (";
  msg_len += str_buf_marshal_str(msg, msg_len, _MSG_CAP, msg_part, str_len(msg_part));
  msg_len += str_buf_marshal_uint(msg, msg_len, _MSG_CAP, (uptr_t)vaddr);
  msg_part = ",";
  msg_len += str_buf_marshal_str(msg, msg_len, _MSG_CAP, msg_part, str_len(msg_part));
  msg_len += str_buf_marshal_uint(msg, msg_len, _MSG_CAP, (uptr_t)paddr);
  msg_part = "):";
  msg_len += str_buf_marshal_str(msg, msg_len, _MSG_CAP, msg_part, str_len(msg_part));

  msg_len += str_buf_marshal_uint(msg, msg_len, _MSG_CAP, (uptr_t)ptab);
  msg_part = "=";
  msg_len += str_buf_marshal_str(msg, msg_len, _MSG_CAP, msg_part, str_len(msg_part));
  msg_len += str_buf_marshal_uint(msg, msg_len, _MSG_CAP, idx);

  msg_len += str_buf_marshal_terminator(msg, msg_len, _MSG_CAP);
  log_info_len(msg, msg_len);

  kernel_assert(_page_tab_entry_is_inited(ptab, idx) == false);
  *(vptr_t *)entry = paddr;
  entry->present = 1;
  entry->writable = 1;
  entry->huge = 1;

}

base_private void _early_tab_prepare(page_tab_entry_t *tab_base, usz_t max_len, uptr_t alloc_end)
{
  uptr_t page;
  uptr_t next_frame;

  next_frame = ((uptr_t)tab_base) + PAGE_SIZE_4K;

  for (page = 0; page < alloc_end; page += PAGE_SIZE_2M) {
    _early_map_add(tab_base, &next_frame, (vptr_t)page, (vptr_t)page);
    kernel_assert((next_frame - (uptr_t)tab_base) < max_len);
  }
}

/*
base_private void _early_tab_system_verify(page_tab_entry_t *p4, uptr_t target_end)
{
  uptr_t target = 0;
  for (tab_entry_index_t p4_idx = 0; p4_idx < PAGE_TAB_ENTRY_COUNT; p4_idx++) {
    page_tab_entry_t *p4_entry = p4 + p4_idx;
    if(_page_tab_entry_is_inited(p4, p4_idx)) {
      page_tab_entry_t *p3 = _entry_get_paddr(p4_entry);
      kernel_assert(((uptr_t)p3) < 1024 * 1024 * 1024);
      for (tab_entry_index_t p3_idx = 0; p3_idx < PAGE_TAB_ENTRY_COUNT; p3_idx++) {
        page_tab_entry_t *p3_entry = p3 + p3_idx;
        if(_page_tab_entry_is_inited(p3, p3_idx)) {
          page_tab_entry_t *p2 = _entry_get_paddr(p3_entry);
          kernel_assert(((uptr_t)p2) < 1024 * 1024 * 1024);
          for (tab_entry_index_t p2_idx = 0; p2_idx < PAGE_TAB_ENTRY_COUNT; p2_idx++) {
            page_tab_entry_t *p2_entry = p2 + p2_idx;
            if(!_page_tab_entry_is_inited(p2, p2_idx)) {
              break;
            } else {
              uptr_t paddr = (uptr_t)_entry_get_paddr(p2_entry);
              kernel_assert(paddr == target);
              target += PAGE_SIZE_2M;

              base_private const usz_t _MSG_CAP = 80;
              ch_t msg[_MSG_CAP];
              usz_t msg_len;
              const ch_t *msg_part;
              msg_len = 0;
              msg_part = "paddr verified: ";
              msg_len += str_buf_marshal_str(msg, msg_len, _MSG_CAP, msg_part, str_len(msg_part));
              msg_len += str_buf_marshal_uint(msg, msg_len, _MSG_CAP, paddr);
              msg_len += str_buf_marshal_terminator(msg, msg_len, _MSG_CAP);
              log_info_len(msg, msg_len);
            }
          }

        }
      }
    }
  }
  kernel_assert((target + PAGE_SIZE_2M) >= target_end);
}
*/

base_private void _load_tab_system(page_tab_entry_t *p4) {
  /*
  kernel_assert(mm_align_check(p4, PAGE_SIZE_4K));
  page_tab_entry_t *p3 = _entry_get_paddr(p4);
  page_tab_entry_t *p2 = _entry_get_paddr(p3);
  page_tab_entry_t *paddr = _entry_get_paddr(p2);
  kernel_assert(mm_align_check(paddr, PAGE_SIZE_2M));
  _early_tab_system_verify(p4, 
      ((uptr_t )_phy_segs[_phy_segs_count - 1].base) + _phy_segs[_phy_segs_count - 1].len);

  u64_t cr3 = cpu_read_cr3();
  page_tab_entry_t *old_p4 = (page_tab_entry_t *)cr3;
  kernel_assert(mm_align_check(old_p4, PAGE_SIZE_4K));
  page_tab_entry_t *old_p3 = _entry_get_paddr(old_p4);
  page_tab_entry_t *old_p2 = _entry_get_paddr(old_p3);
  page_tab_entry_t *old_paddr = _entry_get_paddr(old_p2);
  kernel_assert(mm_align_check(old_paddr, PAGE_SIZE_2M));
  _early_tab_system_verify(old_p4, 1024 * 1024 * 1024);
*/

  u64_t val;
  
  log_info("_load_tab_system 1");
  kernel_assert(mm_align_check(p4, PAGE_SIZE_4K));

  val = (uptr_t)p4;
  val = val & 0x000ffffffffff000;
  cpu_write_cr3(val);
  log_info("_load_tab_system 2");
}

void page_early_tab_load(uptr_t kernel_start, uptr_t kernel_end)
{
  usz_t alloc_size;
  usz_t selected;
  page_tab_entry_t *p4;
  uptr_t seg_base;
  usz_t seg_size;
  uptr_t seg_end;
  uptr_t alloc_end;
  
  log_info("page_early_tab_load started");

  alloc_end = ((uptr_t )_phy_segs[_phy_segs_count - 1].base) + _phy_segs[_phy_segs_count - 1].len;

  /* How many 2M large pages need to describe physical memory */
  alloc_size = alloc_end / (2 * 1024 * 1024) / 512 * 4096;

  /* 2 times space will be reserved to save higher level page tables */
  alloc_size *= 2;

  p4 = NULL;
  for (selected = 0; selected < _phy_segs_count; selected++) {
    seg_base = (uptr_t)_phy_segs[selected].base;
    seg_size = _phy_segs[selected].len;
    seg_end = seg_base + seg_size;

    /* Do not use too 'low' physical memory */
    if (seg_end > (16 * 1024 * 1024) && seg_size > alloc_size) {
      if (seg_base <= kernel_start && seg_end > kernel_end) {
        p4 = (page_tab_entry_t *)mm_align_up((vptr_t)(kernel_end + 1), PAGE_SIZE_4K);
        if ((seg_end - (uptr_t)p4) > alloc_size) {
          break;
        } else {
          p4 = NULL;
        }
      } else {
        kernel_assert(seg_base > kernel_end || seg_end < kernel_start);
        p4 = (page_tab_entry_t *)mm_align_up((vptr_t)seg_base, PAGE_SIZE_4K);
        if ((seg_end - (uptr_t)p4) > alloc_size) {
          break;
        } else {
          p4 = NULL;
        }
      }
    }
  }

  if(selected >= _phy_segs_count) {
    kernel_panic("Can not find proper memory space to deposit early stage page table");
  }
  kernel_assert(selected < _phy_segs_count);
  kernel_assert(p4 != NULL);

  log_info("page_early_tab_load 1");

  mm_clean(p4, PAGE_SIZE_4K);

  log_info("page_early_tab_load 2");

  _early_tab_prepare(p4, seg_end - (uptr_t)p4, alloc_end);

  base_private const usz_t _MSG_CAP = 80;
  ch_t msg[_MSG_CAP];
  usz_t msg_len;
  const ch_t *msg_part;

  msg_len = 0;
  msg_part = "kerenl start: ";
  msg_len += str_buf_marshal_str(msg, msg_len, _MSG_CAP, msg_part, str_len(msg_part));
  msg_len += str_buf_marshal_uint(msg, msg_len, _MSG_CAP, kernel_start);
  msg_part = ", end: ";
  msg_len += str_buf_marshal_str(msg, msg_len, _MSG_CAP, msg_part, str_len(msg_part));
  msg_len += str_buf_marshal_uint(msg, msg_len, _MSG_CAP, kernel_end);
  msg_part = ", p4: ";
  msg_len += str_buf_marshal_str(msg, msg_len, _MSG_CAP, msg_part, str_len(msg_part));
  msg_len += str_buf_marshal_uint(msg, msg_len, _MSG_CAP, (u64_t)p4);
  msg_len += str_buf_marshal_terminator(msg, msg_len, _MSG_CAP);
  
  log_info_len(msg, msg_len);

  _load_tab_system(p4);

  log_info("page_early_tab_load 4");
}
