#include "containers_string.h"
#include "kernel_panic.h"
#include "log.h"
#include "mm_private.h"

base_private uptr_t _kernel_start;
base_private uptr_t _kernel_end;

/* @see
 * https://en.wikipedia.org/wiki/Executable_and_Linkable_Format#Section_header  
 */
typedef struct multiboot_elf_sections_entry {
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
} base_struct_packed multiboot_elf_sections_entry_t;

typedef struct multiboot_tag_elf_sections {
  uint32_t num;
  uint32_t section_size;
  uint32_t shndx;
  multiboot_elf_sections_entry_t sections[];
} base_struct_packed multiboot_tag_elf_sections_t;

base_private inline bo_t _math_is_pow2(u64_t n)
{
  return base_likely(!((n) & ((n)-1)));
}

void mm_copy(byte_t *dest, const byte_t *src, usz_t copy_len)
{
  for (usz_t i = 0; i < copy_len; i++) {
    dest[i] = src[i];
  }
}

void mm_clean(vptr_t mem, usz_t size)
{
  kernel_assert(size > 0);
  for (usz_t i = 0; i < size; i++) {
    ((byte_t *)mem)[i] = 0;
  }
}

vptr_t mm_align_up(vptr_t p, u64_t align)
{
  uptr_t intp = (uptr_t)p;
  u64_t a = align - 1;
  kernel_assert(align > 0);
  kernel_assert(_math_is_pow2(align));
  intp = ((intp + a) & ~a);
  return (vptr_t)intp;
}

bo_t mm_align_check(vptr_t p, u64_t align)
{
  return ((uptr_t)p) % align == 0;
}

void mm_init(const byte_t *kernel_elf_info,
    usz_t elf_info_len base_may_unuse,
    const byte_t *mmap_info,
    usz_t mmap_info_len)
{
  base_private const usz_t _MSG_CAP = 80;
  ch_t msg[_MSG_CAP];
  usz_t msg_len;
  const ch_t *msg_part;

  multiboot_tag_elf_sections_t *secs;
  multiboot_elf_sections_entry_t *entry;
  usz_t sec_cnt;
  usz_t sec_size;

  log_info("mm_init started");

  mm_page_init_mmap_info(mmap_info, mmap_info_len);

  log_info("mm_page_init_mmap_info finished");

  secs = (multiboot_tag_elf_sections_t *)kernel_elf_info;
  sec_cnt = secs->num;
  sec_size = secs->section_size;

  _kernel_start = U64_MAX;
  _kernel_end = 0;

  for (usz_t i = 0; i < sec_cnt; i++) {
    entry = (multiboot_elf_sections_entry_t *)(((uptr_t)secs->sections) +
                                               i * sec_size);

    if (entry->type != 0) {
      if (entry->addr < _kernel_start) {
        _kernel_start = entry->addr;
      }
      if ((entry->size + entry->addr) > _kernel_end) {
        _kernel_end = entry->size + entry->addr;
      }
    }

    msg_len = 0;
    msg_len += str_buf_marshal_uint(msg, msg_len, _MSG_CAP, i);
    msg_part = ": addr=";
    msg_len += str_buf_marshal_str(
        msg, msg_len, _MSG_CAP, msg_part, str_len(msg_part));
    msg_len += str_buf_marshal_uint(msg, msg_len, _MSG_CAP, entry->addr);
    msg_part = ", type=";
    msg_len += str_buf_marshal_str(
        msg, msg_len, _MSG_CAP, msg_part, str_len(msg_part));
    msg_len += str_buf_marshal_uint(msg, msg_len, _MSG_CAP, entry->type);
    msg_part = ", size=";
    msg_len += str_buf_marshal_str(
        msg, msg_len, _MSG_CAP, msg_part, str_len(msg_part));
    msg_len += str_buf_marshal_uint(msg, msg_len, _MSG_CAP, entry->size);
    str_buf_marshal_terminator(msg, msg_len, _MSG_CAP);

    log_info_len(msg, msg_len);
  }

  /* Kernel image is impossible to be that large */
  kernel_assert((_kernel_end - _kernel_start) < 1024 * 1024 * 1024);
  /* Verify this function must be with in kernel image */
  kernel_assert((uptr_t)mm_init < _kernel_end);
  kernel_assert((uptr_t)mm_init > _kernel_start);
  kernel_assert(mm_page_phy_addr_range_valid(_kernel_start, _kernel_end));

  msg_len = 0;
  msg_part = "kernel start =";
  msg_len += str_buf_marshal_str(
        msg, msg_len, _MSG_CAP, msg_part, str_len(msg_part));
  msg_len += str_buf_marshal_uint(msg, msg_len, _MSG_CAP, _kernel_start);
  msg_part = ", end =";
  msg_len += str_buf_marshal_str(
        msg, msg_len, _MSG_CAP, msg_part, str_len(msg_part));
  msg_len += str_buf_marshal_uint(msg, msg_len, _MSG_CAP, _kernel_end);
  msg_len += str_buf_marshal_terminator(msg, msg_len, _MSG_CAP);
  log_info_len(msg, msg_len);
    
  page_early_tab_load(_kernel_start, _kernel_end);
}
