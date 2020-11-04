#include "kernel_main.h"
#include "containers_string.h"
#include "drivers_acpi.h"
#include "drivers_keyboard.h"
#include "drivers_screen.h"
#include "drivers_time.h"
#include "interrupts.h"
#include "kernel_panic.h"
#include "kernel_port.h"
#include "log.h"
#include "mm.h"
#include "panel.h"

typedef enum {
  MULTI_BOOT_TAG_TYPE_MMAP = 6,
  MULTI_BOOT_TAG_TYPE_ELF_SYMBOLS = 9,
} multi_boot_tag_type_t;

#define MULTI_BOOT_INFO_SLOT_COUNT 128
typedef struct {
  const byte_t *addr;
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

base_private const byte_t *_boot_info_process_tag(
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
  case 6:
    /*process_boot_info_mem_map(ptr, size, row_no);*/
    break;
  case 8: /* Frame buffer information */
    break;
  case 9: /* ELF symbols */
    break;
  case 10: /* Advanced power management */
    break;
  case 14: /* ACPI old RSDP */
  case 15: /* ACPI new RSDP */
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

base_private const byte_t *_boot_info_process_tag_header(
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
base_private void _boot_info_process(multi_boot_info_t *info)
{
  /* Multiboot 2 boot information is defined by specification:
   * https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html
   *         #Boot-information-format*/
  u32_t reserved;
  u32_t type;
  u32_t size;
  const byte_t *ptr = info->addr;

  info->total_size = *(u32_t *)ptr;
  ptr += 4;
  reserved = *(u32_t *)ptr;
  ptr += 4;
  kernel_assert(reserved == 0);

  while (((uptr_t)ptr - ((uptr_t)info->addr)) < (info->total_size)) {
    ptr = _boot_info_process_tag_header(ptr, &type, &size);
    if (type == 0) {
      break;
    } else {
      ptr = _boot_info_process_tag(&_boot_info, ptr, type, size);
    }

    ptr = (const byte_t *)mm_align_up((vptr_t)ptr, 8);
  }

  kernel_assert(((uptr_t)ptr - ((uptr_t)info->addr)) == (info->total_size));
}

base_private void _boot_info_init(multi_boot_info_t *info, const byte_t *addr)
{
  info->addr = addr;
  for (usz_t i = 0; i < MULTI_BOOT_INFO_SLOT_COUNT; i++) {
    info->ptrs[i] = NULL;
    info->lens[i] = 0;
  }
}

void kernal_main(u64_t addr)
{
  screen_init();

  _boot_info_init(&_boot_info, (const byte_t *)addr);
  _boot_info_process(&_boot_info);

  panel_start();

  kernel_assert(_boot_info.ptrs[MULTI_BOOT_TAG_TYPE_ELF_SYMBOLS] != NULL);
  kernel_assert(_boot_info.lens[MULTI_BOOT_TAG_TYPE_ELF_SYMBOLS] != 0);
  kernel_assert(_boot_info.ptrs[MULTI_BOOT_TAG_TYPE_MMAP] != NULL);
  kernel_assert(_boot_info.lens[MULTI_BOOT_TAG_TYPE_MMAP] != 0);
  mm_init(_boot_info.ptrs[MULTI_BOOT_TAG_TYPE_ELF_SYMBOLS],
      _boot_info.lens[MULTI_BOOT_TAG_TYPE_ELF_SYMBOLS],
      _boot_info.ptrs[MULTI_BOOT_TAG_TYPE_MMAP],
      _boot_info.lens[MULTI_BOOT_TAG_TYPE_MMAP]);

  intr_init();
  time_init();
  keyboard_init();
  intr_irq_enable();

  // acpi_init(ptr, size - 8); /* Minus header length */

  /* Some temp tests following ***********************************************/

  /* Test able to acess pointer NULL, this behaviour may be protected by a 
   * guard page 
  ch_t *str = NULL;
  str[0] = 'A';
  str[1] = '\0';
  log_info(str, str_len(str));
  */

  while (1) {
    /* This allows the CPU to enter a sleep state in which it consumes much
     * less energy. See: https://en.wikipedia.org/wiki/HLT_(x86_instruction) */
    __asm__("hlt");
  }
}
