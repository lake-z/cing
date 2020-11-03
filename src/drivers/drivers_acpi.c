#include "drivers_acpi.h"
#include "containers_string.h"
#include "kernel_panic.h"
#include "log.h"

void acpi_init(const byte_t *multi_boot_info, usz_t len)
{
  uptr_t ptr = (uptr_t)multi_boot_info;
  const ch_t *signature;
  u8_t check_sum;
  const ch_t *oem;
  u8_t revision;
  base_private const usz_t _MSG_CAP = 128;
  ch_t msg[_MSG_CAP];
  usz_t msg_len;
  u32_t rsdt_addr;
  u64_t byte_sum;

  /* Multi boot info is pointing to a RSDP (Root System Description Pointer) */
  signature = (const ch_t *)ptr;
  log_info(signature, 8);
  ptr += 8;

  check_sum = *(const u8_t *)ptr;
  ptr += 1;
  (void)check_sum;

  oem = (const ch_t *)ptr;
  log_info(oem, 6);
  ptr += 6;

  revision = *(const u8_t *)ptr;
  ptr += 1;
  msg_len = 0;
  msg_len += str_buf_marshal_uint(msg, msg_len, _MSG_CAP, revision);
  msg_len += str_buf_marshal_terminator(msg, msg_len, _MSG_CAP);
  log_info(msg, msg_len);

  rsdt_addr = *(const u32_t *)ptr;
  ptr += 4;
  msg_len = 0;
  msg_len += str_buf_marshal_uint(msg, msg_len, _MSG_CAP, rsdt_addr);
  msg_len += str_buf_marshal_terminator(msg, msg_len, _MSG_CAP);
  log_info(msg, msg_len);

  msg_len = 0;
  msg_len += str_buf_marshal_uint(
      msg, msg_len, _MSG_CAP, ptr - (uptr_t)multi_boot_info);
  msg_len += str_buf_marshal_terminator(msg, msg_len, _MSG_CAP);
  log_info(msg, msg_len);

  byte_sum = 0;
  for (const byte_t *byte = multi_boot_info; (uptr_t)byte < ptr; byte++) {
    byte_sum += *byte;
  }

  kernel_assert((ptr - (uptr_t)multi_boot_info) == len);
  kernel_assert((byte_sum & 0xff) == 0); /* Validate check sum */
  kernel_assert(revision == 0); /* TODO: ACPI 2.0+ needs to be supported */
}
