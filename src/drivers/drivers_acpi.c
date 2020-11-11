#include "drivers_acpi.h"
#include "containers_string.h"
#include "kernel_panic.h"
#include "log.h"

typedef struct sdt_header {
  ch_t signature[4];
  u32_t len;
  u8_t revision;
  u8_t checksum;
  ch_t oemid[6];
  ch_t oem_tab_id[8];
  u32_t oem_revision;
  u32_t creator_id;
  u32_t creator_revision;
} base_struct_packed sdt_header_t;

base_private void _init_rsdt(byte_t *rsdt)
{
  sdt_header_t *sdt = (sdt_header_t *)rsdt;
  usz_t entry_cnt = (sdt->len - sizeof(sdt_header_t)) / 4;
 
  log_info_ln_len(sdt->signature, 4);

  for (usz_t i = 0; i < entry_cnt; i++) {
    u32_t ptr32 = *(u32_t *)(rsdt + sizeof(sdt_header_t) + i * 4);
    sdt = (sdt_header_t *)(uptr_t)ptr32;
    log_info_ln_len(sdt->signature, 4);
  }
}

void acpi_init(const byte_t *multi_boot_info, usz_t len)
{
  uptr_t ptr = (uptr_t)multi_boot_info;
  const ch_t *signature;
  u8_t check_sum;
  u8_t revision;
  base_private const usz_t _MSG_CAP = 128;
  ch_t msg[_MSG_CAP];
  const ch_t *msg_part;
  usz_t msg_len;
  u32_t rsdt_addr;
  u64_t byte_sum;

  /* Multi boot info is pointing to a RSDP (Root System Description Pointer) */
  signature = (const ch_t *)ptr;
  log_info_ln_len(signature, 8);
  ptr += 8;

  check_sum = *(const u8_t *)ptr;
  ptr += 1;
  (void)check_sum;

  /* oem */
  ptr += 6;

  revision = *(const u8_t *)ptr;
  ptr += 1;
  msg_len = 0;
  msg_len += str_buf_marshal_uint(msg, msg_len, _MSG_CAP, revision);
  msg_len += str_buf_marshal_terminator(msg, msg_len, _MSG_CAP);
  log_info_ln_len(msg, msg_len);

  rsdt_addr = *(const u32_t *)ptr;
  ptr += 4;
  msg_len = 0;
  msg_part = "RSDT: ";
  msg_len += str_buf_marshal_str(msg, msg_len, _MSG_CAP, msg_part, str_len(msg_part));
  msg_len += str_buf_marshal_uint(msg, msg_len, _MSG_CAP, rsdt_addr);
  msg_len += str_buf_marshal_terminator(msg, msg_len, _MSG_CAP);
  log_info_ln_len(msg, msg_len);

  msg_len = 0;
  msg_len += str_buf_marshal_uint(
      msg, msg_len, _MSG_CAP, ptr - (uptr_t)multi_boot_info);
  msg_len += str_buf_marshal_terminator(msg, msg_len, _MSG_CAP);
  log_info_ln_len(msg, msg_len);

  byte_sum = 0;
  for (const byte_t *byte = multi_boot_info; (uptr_t)byte < ptr; byte++) {
    byte_sum += *byte;
  }

  kernel_assert((ptr - (uptr_t)multi_boot_info) == len);
  kernel_assert((byte_sum & 0xff) == 0); /* Validate check sum */
  kernel_assert(revision == 0); /* TODO: ACPI 2.0+ needs to be supported */

  _init_rsdt((byte_t *)(u64_t)rsdt_addr);
}

