#include "kernel_main.h"
#include "containers_string.h"
#include "drivers_keyboard.h"
#include "drivers_screen.h"
#include "drivers_time.h"
#include "drivers_serial.h"
#include "interrupts.h"
#include "kernel_panic.h"
#include "drivers_port.h"
#include "log.h"
#include "mm.h"
#include "panel.h"

#define _MULTI_BOOT_TAG_TYPE_MMAP 6
#define _MULTI_BOOT_TAG_TYPE_ELF_SYMBOLS 9
#define _MULTI_BOOT_TAG_TYPE_ACPI_OLD 14
#define _MULTI_BOOT_TAG_TYPE_ACPI_NEW 15

#define MULTI_BOOT_INFO_SLOT_COUNT 128
#define MULTI_BOOT_INFO_CAP (1024 * 1024)
typedef struct {
  const byte_t info[MULTI_BOOT_INFO_CAP];
  usz_t total_size;
  const byte_t *ptrs[MULTI_BOOT_INFO_SLOT_COUNT];
  usz_t lens[MULTI_BOOT_INFO_SLOT_COUNT];
} multi_boot_info_t;

base_private multi_boot_info_t _boot_info;

/* Forwarded declarations */
void process_boot_info_str(
    const byte_t *addr, usz_t size base_may_unuse, usz_t *row_no) base_no_null;
void process_boot_info_mem_map(
    const byte_t *ptr, usz_t size base_may_unuse, usz_t *row_no);

void process_boot_info_str(
    const byte_t *addr, usz_t size base_may_unuse, usz_t *row_no)
{
  usz_t len = str_len((const ch_t *)addr);

  kernel_assert((len + 1 + 8) == size);
  screen_write_str(
      (const ch_t *)addr, SCREEN_COLOR_WHITE, SCREEN_COLOR_BLACK, *row_no, 0);
  (*row_no) += 1;
}

base_private const byte_t *_multi_boot_info_save_tag(
    multi_boot_info_t *info, const byte_t *ptr, u32_t type, u32_t size)
{
  base_private const usz_t _MSG_LEN = 128;
  ch_t msg[_MSG_LEN];
  usz_t msg_ptr;
  ch_t *msg_part;

  switch (type) {
  case 1: /* Boot command line */
  case 2: /* Boot loader name
    process_boot_info_str(ptr, size, row_no); */
    break;
  case 4: /* Old memory information */
    break;
  case 5: /* BIOS boot device */
    break;
  case _MULTI_BOOT_TAG_TYPE_MMAP:
    /*process_boot_info_mem_map(ptr, size, row_no);*/
    break;
  case 8: /* Frame buffer information */
    break;
  case _MULTI_BOOT_TAG_TYPE_ELF_SYMBOLS: /* ELF symbols */
    break;
  case 10: /* Advanced power management */
    break;
  case _MULTI_BOOT_TAG_TYPE_ACPI_OLD: /* ACPI old RSDP */
  case _MULTI_BOOT_TAG_TYPE_ACPI_NEW: /* ACPI new RSDP */
    break;
  default:
    msg_part = (ch_t *)"Invalid multiboot info tag type: ";
    msg_ptr = 0;
    msg_ptr += str_buf_marshal_str(
        msg, msg_ptr, _MSG_LEN, msg_part, str_len(msg_part));
    msg_ptr += str_buf_marshal_uint(msg, msg_ptr, _MSG_LEN, type);
    msg[msg_ptr] = '\0';
    kernel_panic(msg);
    break;
  }

  kernel_assert(type < MULTI_BOOT_INFO_SLOT_COUNT);
  kernel_assert(info->ptrs[type] == NULL);
  kernel_assert(info->lens[type] == 0);
  info->lens[type] = size - 8;
  info->ptrs[type] = ptr;
  return ptr + size - 8; /* 8 bytes of header is already passed */
}

base_private const byte_t *_multi_boot_info_process_tag_header(
    const byte_t *ptr, u32_t *type, u32_t *size)
{
  kernel_assert((((uptr_t)ptr) % 8) == 0);

  *type = *(u32_t *)ptr;
  ptr += 4;
  *size = *(u32_t *)ptr;
  ptr += 4;

  return ptr;
}

/* multi_boot_info_t must be initialized before calling this function */
base_private void _multi_boot_info_save(multi_boot_info_t *info, const byte_t *boot)
{
  /* Multiboot 2 boot information is defined by specification:
   * https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html
   *         #Boot-information-format*/
  u32_t reserved;
  u32_t type;
  u32_t size;
  const byte_t *ptr = boot;

  info->total_size = *(u32_t *)ptr;
  ptr += 4;
  reserved = *(u32_t *)ptr;
  ptr += 4;
  kernel_assert((info->total_size) < MULTI_BOOT_INFO_CAP);
  kernel_assert(reserved == 0);

  /* Initialize fields */
  mm_copy((byte_t *)info->info, boot, info->total_size);
  for (usz_t i = 0; i < MULTI_BOOT_INFO_SLOT_COUNT; i++) {
    info->ptrs[i] = NULL;
    info->lens[i] = 0;
  }
  ptr = info->info + 8;

  while (((uptr_t)ptr - ((uptr_t)info->info)) < (info->total_size)) {
    ptr = _multi_boot_info_process_tag_header(ptr, &type, &size);
    if (type == 0) {
      break;
    } else {
      ptr = _multi_boot_info_save_tag(&_boot_info, ptr, type, size);
    }

    ptr = (const byte_t *)mm_align_up((vptr_t)ptr, 8);
  }

  kernel_assert(((uptr_t)ptr - ((uptr_t)info->info)) == (info->total_size));
}

base_private base_no_return _kernel_halt(void)
{
  while (1) {
    __asm__("hlt");
  }
}

void kernal_main(uptr_t multi_boot_info)
{
  serial_init();

  log_line_start(LOG_LEVEL_INFO);
  log_str(LOG_LEVEL_INFO, "kernel_prototype started..");
  log_line_end(LOG_LEVEL_INFO);

  screen_init();

  _multi_boot_info_save(&_boot_info, (const byte_t *)multi_boot_info);

  kernel_assert(_boot_info.ptrs[_MULTI_BOOT_TAG_TYPE_ELF_SYMBOLS] != NULL);
  kernel_assert(_boot_info.lens[_MULTI_BOOT_TAG_TYPE_ELF_SYMBOLS] != 0);
  kernel_assert(_boot_info.ptrs[_MULTI_BOOT_TAG_TYPE_MMAP] != NULL);
  kernel_assert(_boot_info.lens[_MULTI_BOOT_TAG_TYPE_MMAP] != 0);
  mm_early_init(_boot_info.ptrs[_MULTI_BOOT_TAG_TYPE_ELF_SYMBOLS],
      _boot_info.lens[_MULTI_BOOT_TAG_TYPE_ELF_SYMBOLS],
      _boot_info.ptrs[_MULTI_BOOT_TAG_TYPE_MMAP],
      _boot_info.lens[_MULTI_BOOT_TAG_TYPE_MMAP]);

  _kernel_halt();


  /*


  intr_init();
  time_init();
  keyboard_init();
  intr_irq_enable();

  panel_start();

  base_private const usz_t _MSG_CAP = 80;
  ch_t msg[_MSG_CAP];
  usz_t msg_len;
  const ch_t *msg_part;

  msg_len = 0;
  msg_part = "boot info: ";
  msg_len += str_buf_marshal_str(msg, msg_len, _MSG_CAP, msg_part, str_len(msg_part));
  msg_len += str_buf_marshal_uint(msg, msg_len, _MSG_CAP, (u64_t)addr);
  msg_part = ", elf symbols: ";
  msg_len += str_buf_marshal_str(msg, msg_len, _MSG_CAP, msg_part, str_len(msg_part));
  msg_len += str_buf_marshal_uint(msg, msg_len, _MSG_CAP, (u64_t)_boot_info.ptrs[MULTI_BOOT_TAG_TYPE_ELF_SYMBOLS]);
  msg_part = ", mmap: ";
  msg_len += str_buf_marshal_str(msg, msg_len, _MSG_CAP, msg_part, str_len(msg_part));
  msg_len += str_buf_marshal_uint(msg, msg_len, _MSG_CAP, (u64_t)_boot_info.ptrs[MULTI_BOOT_TAG_TYPE_MMAP]);
  msg_len += str_buf_marshal_terminator(msg, msg_len, _MSG_CAP);
  log_info_str_line_len(msg, msg_len);

  kernel_assert(_boot_info.ptrs[MULTI_BOOT_TAG_TYPE_ACPI_OLD] != NULL);
  kernel_assert(_boot_info.lens[MULTI_BOOT_TAG_TYPE_ACPI_OLD] != 0);
  acpi_init(_boot_info.ptrs[MULTI_BOOT_TAG_TYPE_ACPI_OLD], 
      _boot_info.lens[MULTI_BOOT_TAG_TYPE_ACPI_OLD]);
  */

  /* Some temp tests following ***********************************************/

  /* Test able to acess pointer NULL, this behaviour may be protected by a 
   * guard page 
  ch_t *str = NULL;
  str[0] = 'A';
  str[1] = '\0';
  log_info(str, str_len(str));
  */
}
