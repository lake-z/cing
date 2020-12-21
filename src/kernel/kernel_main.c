#include "containers_string.h"
#include "cpu.h"
#include "drivers_acpi.h"
#include "drivers_keyboard.h"
#include "drivers_port.h"
#include "drivers_screen.h"
#include "drivers_serial.h"
#include "drivers_time.h"
#include "interrupts.h"
#include "kernel_panic.h"
#include "log.h"
#include "mm.h"
#include "panel.h"

/* Defined in boot/boot.asm */
extern uptr_t boot_stack_bottom;
extern uptr_t boot_stack_top;

/* Called from boot/boot.asm */
void kernal_main(uptr_t multi_boot_info);

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
base_private void _multi_boot_info_save(
    multi_boot_info_t *info, const byte_t *boot)
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

    ptr = (const byte_t *)mm_align_up((uptr_t)ptr, 8);
  }

  kernel_assert(((uptr_t)ptr - ((uptr_t)info->info)) == (info->total_size));
}

base_private base_no_return _kernel_halt(void)
{
  while (1) {
    __asm__("hlt");
  }
}

base_private void _test_stack_overflow(void)
{
  byte_t stack_data[1000];

  /*
  log_line_start(LOG_LEVEL_DEBUG);
  log_str(LOG_LEVEL_DEBUG, "stack at: ");
  log_uint_of_size(LOG_LEVEL_DEBUG, (uptr_t)stack_data);
  log_line_end(LOG_LEVEL_DEBUG);
  */
  if (((uptr_t)stack_data - 2048) > mm_vadd_stack_bp_top_get()) {
    _test_stack_overflow();
  }
}

void kernal_main(uptr_t multi_boot_info)
{
  uptr_t rbp;
  uptr_t rsp;
  u64_t rbp_off;
  u64_t rsp_off;
  uptr_t new_rsp;
  uptr_t new_rbp;
  uptr_t curr_stack_top;
  uptr_t curr_stack_bottom;
  usz_t curr_stack_len;

  screen_init();
  serial_init();

  log_line_start(LOG_LEVEL_INFO);
  log_str(LOG_LEVEL_INFO, "kernel_prototype started..");
  log_line_end(LOG_LEVEL_INFO);

  log_line_start(LOG_LEVEL_INFO);
  log_str(LOG_LEVEL_INFO, "git revision: ");
  log_str(LOG_LEVEL_INFO, BUILD_GIT_REVISION);
  log_line_end(LOG_LEVEL_INFO);

  _multi_boot_info_save(&_boot_info, (const byte_t *)multi_boot_info);

  kernel_assert(_boot_info.ptrs[_MULTI_BOOT_TAG_TYPE_ELF_SYMBOLS] != NULL);
  kernel_assert(_boot_info.lens[_MULTI_BOOT_TAG_TYPE_ELF_SYMBOLS] != 0);
  kernel_assert(_boot_info.ptrs[_MULTI_BOOT_TAG_TYPE_MMAP] != NULL);
  kernel_assert(_boot_info.lens[_MULTI_BOOT_TAG_TYPE_MMAP] != 0);
  mm_early_init(_boot_info.ptrs[_MULTI_BOOT_TAG_TYPE_ELF_SYMBOLS],
      _boot_info.lens[_MULTI_BOOT_TAG_TYPE_ELF_SYMBOLS],
      _boot_info.ptrs[_MULTI_BOOT_TAG_TYPE_MMAP],
      _boot_info.lens[_MULTI_BOOT_TAG_TYPE_MMAP]);

  if (_boot_info.ptrs[_MULTI_BOOT_TAG_TYPE_ACPI_OLD] != NULL) {
    kernel_assert(_boot_info.lens[_MULTI_BOOT_TAG_TYPE_ACPI_OLD] != 0);
    acpi_init_old(_boot_info.ptrs[_MULTI_BOOT_TAG_TYPE_ACPI_OLD],
        _boot_info.lens[_MULTI_BOOT_TAG_TYPE_ACPI_OLD]);
  } else {
    kernel_assert(_boot_info.ptrs[_MULTI_BOOT_TAG_TYPE_ACPI_NEW] != NULL);
    kernel_assert(_boot_info.lens[_MULTI_BOOT_TAG_TYPE_ACPI_NEW] != 0);
    acpi_init_new(_boot_info.ptrs[_MULTI_BOOT_TAG_TYPE_ACPI_NEW],
        _boot_info.lens[_MULTI_BOOT_TAG_TYPE_ACPI_NEW]);
  }

  intr_init();

  mm_init((uptr_t)&boot_stack_bottom, (uptr_t)&boot_stack_top);

  /* Switching from boot time stack to new stack in high half. 
   * It is safer to do it in kernel_main as it is the entry function of C code,
   * and will never get return. */
  curr_stack_top = (uptr_t)&boot_stack_top;
  curr_stack_bottom = (uptr_t)&boot_stack_bottom;

  /* Read from register RSP and RBP */
  __asm__("movq %%rbp, %0" : "=r"(rbp) :);
  __asm__("movq %%rsp, %0" : "=r"(rsp) :);

  rbp_off = (curr_stack_bottom - rbp);
  rsp_off = (curr_stack_bottom - rsp);

  new_rbp = mm_vadd_stack_bp_bottom_get() - rbp_off;
  new_rsp = mm_vadd_stack_bp_bottom_get() - rsp_off;

  curr_stack_len = curr_stack_bottom - curr_stack_top;

  /* Copy everything from old stack to new stack */
  mm_copy((byte_t *)(mm_vadd_stack_bp_bottom_get() - curr_stack_len),
      (byte_t *)curr_stack_top, curr_stack_len);

  /* Set register RSP and RBP to new value, after here we switch to work on
   * new stack. */
  __asm__("movq %0, %%rbp" : /* no output */ : "r"(new_rbp));
  __asm__("movq %0, %%rsp" : /* no output */ : "r"(new_rsp));
  _test_stack_overflow();

#ifdef BUILD_BUILTIN_TEST_ENABLED
  test_mm();
#endif

  log_line_start(LOG_LEVEL_INFO);
  log_str(LOG_LEVEL_INFO, "kernel_prototype ended.");
  log_line_end(LOG_LEVEL_INFO);

  _kernel_halt();
}
