#include "containers_string.h"
#include "drivers_screen.h"
#include "mm_low.h"
#include "kernal_main.h"
#include "kernel_panic.h"

/* Forwarded declarations */
base_private void process_boot_info_str(
  const byte_t *addr, 
  usz_t size base_may_unuse, 
  usz_t *row_no
) base_no_null;

base_private void process_boot_info_str(
  const byte_t *addr, 
  usz_t size base_may_unuse, 
  usz_t *row_no
)
{
  usz_t len = str_len((const ch_t *)addr);

  kernel_assert((len + 1 + 8) == size);
  screen_write_str((const ch_t *)addr, SCREEN_COLOR_WHITE, SCREEN_COLOR_BLACK, 
    *row_no, 0);
  (*row_no) += 1;
}
base_private void process_boot_info_mem_map(
  const byte_t *ptr, 
  usz_t size base_may_unuse, 
  usz_t *row_no
)
{
  base_private const usz_t _MSG_LEN = 128;
  ch_t msg[_MSG_LEN];
  usz_t msg_ptr;
  ch_t *msg_part;
  u32_t entry_size;
  u32_t entry_ver;
  u64_t entry_cnt;

  entry_size = *(u32_t *)ptr;
  ptr += 4;
  
  entry_ver = *(u32_t *)ptr;
  ptr += 4;

  msg_ptr = 0;
  msg_part = (ch_t *)"entry size: ";
  msg_ptr += str_buf_marshal_str(msg, msg_ptr, _MSG_LEN, msg_part, 
    str_len(msg_part));
  msg_ptr += str_buf_marshal_uint(msg, msg_ptr, _MSG_LEN, entry_size);
  msg_part = (ch_t *)", version: ";
  msg_ptr += str_buf_marshal_str(msg, msg_ptr, _MSG_LEN, msg_part, 
    str_len(msg_part));
  msg_ptr += str_buf_marshal_uint(msg, msg_ptr, _MSG_LEN, entry_ver);
  msg_ptr += str_buf_marshal_terminator(msg, msg_ptr, _MSG_LEN);
  screen_write_str(msg, SCREEN_COLOR_WHITE, SCREEN_COLOR_BLACK, *row_no, 0);
  *row_no += 1;
  kernel_assert(entry_ver == 0);
  kernel_assert(entry_size == 24);

  kernel_assert((size - 8 - 8) % entry_size == 0);
  entry_cnt = (size - 8 - 8) / entry_size;

  (void)(entry_cnt);
  /* 
  for (u64_t en  = 0; en < entry_cnt; en++) {
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
    ptr += 4;

    msg_ptr = 0;
    msg_part = (ch_t *)"base: ";
    msg_ptr += str_buf_marshal_str(msg, msg_ptr, _MSG_LEN, msg_part, 
      str_len(msg_part));
    msg_ptr += str_buf_marshal_uint(msg, msg_ptr, _MSG_LEN, base);

    msg_part = (ch_t *)", len: ";
    msg_ptr += str_buf_marshal_str(msg, msg_ptr, _MSG_LEN, msg_part, 
      str_len(msg_part));
    msg_ptr += str_buf_marshal_uint(msg, msg_ptr, _MSG_LEN, len);

    msg_part = (ch_t *)", type: ";
    msg_ptr += str_buf_marshal_str(msg, msg_ptr, _MSG_LEN, msg_part, 
      str_len(msg_part));
    msg_ptr += str_buf_marshal_uint(msg, msg_ptr, _MSG_LEN, type);

    msg_ptr += str_buf_marshal_terminator(msg, msg_ptr, _MSG_LEN);

    screen_write_str(msg, SCREEN_COLOR_WHITE, SCREEN_COLOR_BLACK, *row_no, 0);
    *row_no += 1;

    kernel_assert(reserve == 0);
  }
  */
}

base_private const byte_t *process_boot_info_tag_header(
  const byte_t *ptr, 
  u32_t *type,
  u32_t *size,
  usz_t *row_no
)
{
  base_private const usz_t _MSG_LEN = 128;
  ch_t msg[_MSG_LEN];
  usz_t msg_ptr;
  ch_t *msg_part;

  kernel_assert((((uptr_t)ptr) % 8) == 0);
 
  *type = *(u32_t *)ptr;
  ptr += 4;
  *size = *(u32_t *)ptr;
  ptr += 4;

  msg_ptr = 0;
  msg_part = (ch_t *)"type: ";
  msg_ptr += str_buf_marshal_str(msg, msg_ptr, _MSG_LEN, msg_part, 
    str_len(msg_part));
  msg_ptr += str_buf_marshal_uint(msg, msg_ptr, _MSG_LEN, *type);

  msg_part = (ch_t *)", size: ";
  msg_ptr += str_buf_marshal_str(msg, msg_ptr, _MSG_LEN, msg_part, 
    str_len(msg_part));
  msg_ptr += str_buf_marshal_uint(msg, msg_ptr, _MSG_LEN, *size);

  msg[msg_ptr] = '\0';
  screen_write_str(msg, SCREEN_COLOR_WHITE, SCREEN_COLOR_BLACK, *row_no, 0);
  *row_no += 1;

  return ptr;
}

base_private const byte_t *process_boot_info_tag(
  const byte_t *ptr, 
  u32_t type,
  u32_t size, 
  usz_t *row_no)
{
  base_private const usz_t _MSG_LEN = 128;
  ch_t msg[_MSG_LEN];
  usz_t msg_ptr;
  ch_t *msg_part;

  switch(type) {
    case 1: /* Boot command line */
    case 2: /* Boot loader name */
      process_boot_info_str(ptr, size, row_no);
      break;
    case 4: /* Old memory information */
      break;
    case 5: /* BIOS boot device */
      break;
    case 6:
      process_boot_info_mem_map(ptr, size, row_no);
      break;
    case 8: /* Frame buffer information */
      break;
     case 9: /* ELF symbols */
      break;
    case 10: /* Advanced power management */
      break;
    case 14: /* ACPI old RSDP */
      break;
    default:
      msg_part = (ch_t *)"Invalid multiboot info tag type: ";
      msg_ptr = 0;
      msg_ptr += str_buf_marshal_str(msg, msg_ptr, _MSG_LEN, 
        msg_part, str_len(msg_part));
      msg_ptr += str_buf_marshal_uint(msg, msg_ptr, _MSG_LEN, type);
      msg[msg_ptr] = '\0';
      kernel_panic(msg);
      break;
  }
  return ptr + size - 8; /* 8 bytes of header is already passed */
}

base_private void process_boot_info(const byte_t *addr)
{
  /* Multiboot 2 boot information is defined by specification:
   * https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html
   *         #Boot-information-format*/
  base_private const usz_t _MSG_LEN = 128;
  ch_t msg[_MSG_LEN];
  usz_t msg_ptr;
  ch_t *msg_part;
  usz_t row_no;
  u32_t total_size;
  u32_t reserved;
  u32_t type;
  u32_t size;
  const byte_t *ptr = addr;

  row_no = 0;
  screen_write_str("MULTIBOOT INFORMATION", SCREEN_COLOR_LIGHT_GREY, 
    SCREEN_COLOR_BLACK, row_no++, 0);


  total_size = *(u32_t *)ptr;
  ptr += 4;
  reserved = *(u32_t *)ptr;
  ptr += 4;
  kernel_assert(reserved == 0);

  msg_ptr = 0;
  msg_part = (ch_t *)"total_size: ";
  msg_ptr += str_buf_marshal_str(msg, msg_ptr, _MSG_LEN, 
    msg_part, str_len(msg_part));
  msg_ptr += str_buf_marshal_uint(msg, msg_ptr, _MSG_LEN, total_size);
  msg[msg_ptr] = '\0';
  screen_write_str(msg, SCREEN_COLOR_WHITE, SCREEN_COLOR_BLACK, row_no++, 0);

  while ((ptr - addr) < total_size) {
    ptr = process_boot_info_tag_header(ptr, &type, &size, &row_no);
    if(type == 0) {
      break;
    } else {
      ptr = process_boot_info_tag(ptr, type, size, &row_no);
    }

    ptr = (const byte_t *)mm_ptr_align_up((vptr_t)ptr, 8);
  }

  kernel_assert((ptr - addr) == total_size);
  msg_ptr = 0;
  msg_part = (ch_t *)"total processed length: ";
  msg_ptr += str_buf_marshal_str(msg, msg_ptr, _MSG_LEN, 
    msg_part, str_len(msg_part));
  msg_ptr += str_buf_marshal_uint(msg, msg_ptr, _MSG_LEN, (u64_t)(ptr - addr));
  msg[msg_ptr] = '\0';
  screen_write_str(msg, SCREEN_COLOR_WHITE, SCREEN_COLOR_BLACK, row_no++, 0);
}

void kernal_main(u64_t addr)
{
  screen_init();
  screen_clear();

  process_boot_info((uch_t *)addr);

  while (1) {
    /* This allows the CPU to enter a sleep state in which it consumes much
     * less energy. See: https://en.wikipedia.org/wiki/HLT_(x86_instruction) */
    __asm__("hlt");
  }
}

