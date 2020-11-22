#include "drivers_acpi.h"
#include "kernel_panic.h"
#include "log.h"
#include "mm.h"

typedef struct sdt_header {
  ch_t signature[4];
  u32_t len; /* Total size of the table, inclusive of the header. */
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
  log_line_start(LOG_LEVEL_INFO);
  log_str(LOG_LEVEL_INFO, "ACPI init RSDT, ");

  sdt_header_t *sdt = (sdt_header_t *)rsdt;
  usz_t entry_cnt = (sdt->len - sizeof(sdt_header_t)) / 4;

  log_str(LOG_LEVEL_INFO, "signature: ");
  log_str_len(LOG_LEVEL_INFO, sdt->signature, 4);
  log_str(LOG_LEVEL_INFO, ", ");
  log_uint(LOG_LEVEL_INFO, entry_cnt);
  log_str(LOG_LEVEL_INFO, " SDTs found: ");
  log_line_end(LOG_LEVEL_INFO);

  for (usz_t i = 0; i < entry_cnt; i++) {
    u32_t ptr32 = *(u32_t *)(rsdt + sizeof(sdt_header_t) + i * 4);

    sdt = (sdt_header_t *)(uptr_t)ptr32;
    log_line_start(LOG_LEVEL_INFO);
    log_str_len(LOG_LEVEL_INFO, sdt->signature, 4);
    log_line_end(LOG_LEVEL_INFO);
  }
}

void acpi_init(const byte_t *multi_boot_info, usz_t len)
{
  uptr_t ptr = (uptr_t)multi_boot_info;
  const ch_t *signature;
  u8_t revision;
  u32_t rsdt_addr;
  u64_t byte_sum;

  log_line_start(LOG_LEVEL_INFO);
  log_str(LOG_LEVEL_INFO, "ACPI init, ");

  /* Multi boot info is pointing to a RSDP (Root System Description Pointer) */
  signature = (const ch_t *)ptr;
  log_str(LOG_LEVEL_INFO, "signature: ");
  log_str_len(LOG_LEVEL_INFO, signature, 8);
  ptr += 8;

  /* Checksum */
  ptr += 1;

  /* OEM */
  ptr += 6;

  revision = *(const u8_t *)ptr;
  ptr += 1;
  kernel_assert(revision == 0); /* TODO: ACPI 2.0+ needs to be supported */

  rsdt_addr = *(const u32_t *)ptr;
  ptr += 4;
  (void)rsdt_addr;

  kernel_assert((ptr - (uptr_t)multi_boot_info) == len);

  byte_sum = 0;
  for (const byte_t *byte = multi_boot_info; (uptr_t)byte < ptr; byte++) {
    byte_sum += *byte;
  }
  kernel_assert((byte_sum & 0xff) == 0); /* Validate check sum */

  log_line_end(LOG_LEVEL_INFO);

  _init_rsdt((byte_t *)(u64_t)rsdt_addr);
}
