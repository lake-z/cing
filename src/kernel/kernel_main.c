#include "containers_string.h"
#include "cpu.h"
#include "drivers_acpi.h"
#include "drivers_keyboard.h"
#include "drivers_nvme.h"
#include "drivers_port.h"
#include "drivers_serial.h"
#include "drivers_time.h"
#include "drivers_vesa.h"
#include "interrupts.h"
#include "kernel_panic.h"
#include "log.h"
#include "mem.h"
#include "tui.h"
#include "video.h"

/* Defined in boot/boot.asm */
extern uptr_t boot_stack_bottom;
extern uptr_t boot_stack_top;

/* Called from boot/boot.asm */
void kernal_main(uptr_t multi_boot_info);

#define _MULTI_BOOT_TAG_TYPE_MMAP 6
#define _MULTI_BOOT_TAG_TYPE_VBE 7
#define _MULTI_BOOT_TAG_TYPE_FRAME_BUFFER 8
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
  case _MULTI_BOOT_TAG_TYPE_VBE:
    break;
  case _MULTI_BOOT_TAG_TYPE_FRAME_BUFFER:
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
   *         #Boot-information-format */
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

  if (((uptr_t)stack_data - 2048) > mm_va_stack_top()) {
    _test_stack_overflow();
  }
}

void kernal_main(uptr_t multi_boot_info)
{
  serial_init();

  log_line_format(LOG_LEVEL_INFO, "cold_spot started..");
  log_line_format(LOG_LEVEL_INFO, "git revision: %s", BUILD_GIT_REVISION);

  _multi_boot_info_save(&_boot_info, (const byte_t *)multi_boot_info);

  kernel_assert(_boot_info.ptrs[_MULTI_BOOT_TAG_TYPE_ELF_SYMBOLS] != NULL);
  kernel_assert(_boot_info.lens[_MULTI_BOOT_TAG_TYPE_ELF_SYMBOLS] != 0);
  kernel_assert(_boot_info.ptrs[_MULTI_BOOT_TAG_TYPE_MMAP] != NULL);
  kernel_assert(_boot_info.lens[_MULTI_BOOT_TAG_TYPE_MMAP] != 0);
  mem_bootstrap_1(_boot_info.ptrs[_MULTI_BOOT_TAG_TYPE_ELF_SYMBOLS],
      _boot_info.lens[_MULTI_BOOT_TAG_TYPE_ELF_SYMBOLS],
      _boot_info.ptrs[_MULTI_BOOT_TAG_TYPE_MMAP],
      _boot_info.lens[_MULTI_BOOT_TAG_TYPE_MMAP]);

  log_enable_video_write();

  //  uptr_t rbp;
  //  uptr_t rsp;
  //  u64_t rbp_off;
  //  u64_t rsp_off;
  //  uptr_t new_rsp;
  //  uptr_t new_rbp;
  //  uptr_t curr_stack_top;
  //  uptr_t curr_stack_bottom;
  //  usz_t curr_stack_len;

  kernel_assert(_boot_info.ptrs[_MULTI_BOOT_TAG_TYPE_FRAME_BUFFER] != NULL);
  kernel_assert(_boot_info.lens[_MULTI_BOOT_TAG_TYPE_FRAME_BUFFER] != 0);
  d_vesa_bootstrap(_boot_info.ptrs[_MULTI_BOOT_TAG_TYPE_FRAME_BUFFER],
      _boot_info.lens[_MULTI_BOOT_TAG_TYPE_FRAME_BUFFER]);

  if (_boot_info.ptrs[_MULTI_BOOT_TAG_TYPE_ACPI_NEW] != NULL) {
    kernel_assert(_boot_info.lens[_MULTI_BOOT_TAG_TYPE_ACPI_NEW] != 0);
    acpi_bootstrap_64(_boot_info.ptrs[_MULTI_BOOT_TAG_TYPE_ACPI_NEW],
        _boot_info.lens[_MULTI_BOOT_TAG_TYPE_ACPI_NEW]);
  } else {
    kernel_assert(_boot_info.ptrs[_MULTI_BOOT_TAG_TYPE_ACPI_OLD] != NULL);
    kernel_assert(_boot_info.lens[_MULTI_BOOT_TAG_TYPE_ACPI_OLD] != 0);
    acpi_bootstrap_32(_boot_info.ptrs[_MULTI_BOOT_TAG_TYPE_ACPI_OLD],
        _boot_info.lens[_MULTI_BOOT_TAG_TYPE_ACPI_OLD]);
  }

  intr_init();

  log_line_format(LOG_LEVEL_INFO, "Boot stack bottom: %lu, top: %lu",
      (uptr_t)&boot_stack_bottom, (uptr_t)&boot_stack_top);

  mem_bootstrap_2();

  //mem_bootstrap_3();

  //  /* Switching from boot time stack to new stack in high half.
  //   * It is safer to do it in kernel_main as it is the entry function of C code,
  //   * and will never get return. */
  //  curr_stack_top = (uptr_t)&boot_stack_top;
  //  curr_stack_bottom = (uptr_t)&boot_stack_bottom;
  //
  //  /* Read from register RSP and RBP */
  //  __asm__("movq %%rbp, %0" : "=r"(rbp) :);
  //  __asm__("movq %%rsp, %0" : "=r"(rsp) :);
  //
  //  rbp_off = (curr_stack_bottom - rbp);
  //  rsp_off = (curr_stack_bottom - rsp);
  //
  //  new_rbp = mm_va_stack_bottom() - rbp_off;
  //  new_rsp = mm_va_stack_bottom() - rsp_off;
  //
  //  curr_stack_len = curr_stack_bottom - curr_stack_top;
  //
  //  /* Copy everything from old stack to new stack */
  //  mm_copy((byte_t *)(mm_va_stack_bottom() - curr_stack_len),
  //      (byte_t *)curr_stack_top, curr_stack_len);
  //
  //  /* Set register RSP and RBP to new value, after here we switch to work on
  //   * new stack. */
  //  __asm__("movq %0, %%rbp" : /* no output */ : "r"(new_rbp));
  //  __asm__("movq %0, %%rsp" : /* no output */ : "r"(new_rsp));
  //  _test_stack_overflow();
  //
  //#ifdef BUILD_SELF_TEST_ENABLED
  //  test_mm();
  //#endif
  //
  //  d_nvme_bootstrap();
  //
  //  tui_start();
  //

  (void)(_test_stack_overflow);

  log_line_format(LOG_LEVEL_INFO, "cold_spot ended.");

  _kernel_halt();
}
