#include "mm.h"
#include "containers_string.h"
#include "kernel_panic.h"
#include "log.h"

base_private uptr_t _kernel_start;
base_private uptr_t _kernel_end;

/**                                                                                
 * @see                                                                            
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

vptr_t mm_align_up(vptr_t p, u64_t align)
{
  uptr_t intp = (uptr_t)p;
  u64_t a = align - 1;
  kernel_assert(align > 0);
  kernel_assert(_math_is_pow2(align));
  intp = ((intp + a) & ~a);
  return (vptr_t)intp;
}

/* Input multi boot information about kernel ELF sections */
void mm_init(const byte_t *boot_info, usz_t info_len base_may_unuse)
{
  base_private const usz_t _MSG_CAP = 80;
  ch_t msg[_MSG_CAP];
  usz_t msg_len;
  const ch_t *msg_part;

  multiboot_tag_elf_sections_t *secs =
      (multiboot_tag_elf_sections_t *)boot_info;
  multiboot_elf_sections_entry_t *entry;
  usz_t sec_cnt = secs->num;
  usz_t sec_size = secs->section_size;

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

    log_info(msg, msg_len);
  }

  /* Verify this function must be with in kernel image */
  kernel_assert((uptr_t)mm_init < _kernel_end);
  kernel_assert((uptr_t)mm_init > _kernel_start);
}
