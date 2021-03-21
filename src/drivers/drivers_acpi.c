#include "drivers_acpi.h"
#include "drivers_pcie.h"
#include "kernel_panic.h"
#include "log.h"
#include "mm.h"

/* SDT: System description table. */
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

/* XSDT: eXtended System Descriptor Table. */
typedef struct xsdt {
  sdt_header_t header;
  /* 64 bit width SDT pointers follows */
} base_struct_packed xsdt_t;

/* RSDP: Root System Description Pointer, for ACPI spec version 1. */
typedef struct rsdp_descriptor_1 {
  ch_t signature[8];
  u8_t checksum;
  ch_t oem[6];
  /* If @revision field contains 0, then ACPI Version 1.0 is used.
  * For subsequent versions (ACPI version 2.0 to 6.1), the value 2 is used.
  * The exact version of ACPI can be deduced via the FADT table. */
  u8_t revision;
  u32_t rsdt_pddr;
} base_struct_packed rsdp_descriptor_1_t;

/* RSDP for ACPI spec version 2 and later. */
typedef struct rsdp_descriptor_2 {
  rsdp_descriptor_1_t v1_fields;
  u32_t len;
  u64_t xsdt_padd;
  u8_t extend_checksum;
  u8_t reserved[3];
} base_struct_packed rsdp_descriptor_2_t;

base_private void _init_xsdt(xsdt_t *xsdt)
{
  usz_t entry_cnt;

  // FIXME: Verify checksum

  log_line_format(LOG_LEVEL_INFO, "XSDT length: %lu, header: %lu",
      (u64_t)(xsdt->header.len), sizeof(xsdt_t));

  entry_cnt = xsdt->header.len - sizeof(xsdt_t);
  kernel_assert((entry_cnt % 8) == 0);
  entry_cnt /= 8;

  log_line_format(LOG_LEVEL_INFO, "XSDT entry count: %lu", entry_cnt);

  for (usz_t i = 0; i < entry_cnt; i++) {
    i64_t cmp;
    // for (usz_t i = 0; i < 1; i++) {
    sdt_header_t *h =
        *(sdt_header_t **)(((uptr_t)xsdt) + sizeof(xsdt_t) + i * 8);

    log_line_format(LOG_LEVEL_INFO, "%lu: %s", i, h->signature);

    cmp = mm_compare((byte_t *)(h->signature), (byte_t *)"MCFG", 4);
    if (cmp == 0) {
      d_pcie_bootstrap((const byte_t *)((uptr_t)h + 44), h->len - 44);
    }
  }
}

/*
base_private void checksum_validate_old(rsdp_descriptor_1_t *v1)
{
}
*/

void acpi_bootstrap_64(const byte_t *multi_boot_info, usz_t len)
{
  const rsdp_descriptor_2_t *rsdp =
      (const rsdp_descriptor_2_t *)multi_boot_info;
  base_mark_unuse(len);

  log_line_format(
      LOG_LEVEL_INFO, "ACPI 64 signature: %s", rsdp->v1_fields.signature);
  log_line_format(
      LOG_LEVEL_INFO, "ACPI 64 revision: %lu", (u64_t)rsdp->v1_fields.revision);

  // kernel_assert(revision == 2);

  //FIXME: Verify checksum!
  //FIXME: Verify length!

  log_line_format(LOG_LEVEL_INFO, "ACPI 64 XSDT paddr: %lu", rsdp->xsdt_padd);

  _init_xsdt((xsdt_t *)(rsdp->xsdt_padd));

  kernel_panic("TODO: ACPI 64 init");
}

/*****************************************************************************
 *              Functions to support ACPI spec version 1                     *
 *****************************************************************************/
base_private void _init_rsdt(byte_t *rsdt)
{
  sdt_header_t *sdt = (sdt_header_t *)rsdt;
  usz_t entry_cnt = (sdt->len - sizeof(sdt_header_t)) / 4;

  log_line_format(
      LOG_LEVEL_INFO, "ACPI init RSDT, %lu SDTs found: ", entry_cnt);

  for (usz_t i = 0; i < entry_cnt; i++) {
    u32_t ptr32 = *(u32_t *)(rsdt + sizeof(sdt_header_t) + i * 4);
    i64_t cmp;

    sdt = (sdt_header_t *)(uptr_t)ptr32;
    log_line_format(LOG_LEVEL_INFO, "signature: %.4s", sdt->signature);

    cmp = mm_compare((byte_t *)(sdt->signature), (byte_t *)"MCFG", 4);
    if (cmp == 0) {
      d_pcie_bootstrap((const byte_t *)((uptr_t)sdt + 44), sdt->len - 44);
    }
  }
}

void acpi_bootstrap_32(const byte_t *multi_boot_info, usz_t len)
{
  uptr_t ptr = (uptr_t)multi_boot_info;
  const ch_t *signature;
  u8_t revision;
  u32_t rsdt_addr;
  u64_t byte_sum;

  /* Multi boot info is pointing to a RSDP (Root System Description Pointer) */
  signature = (const ch_t *)ptr;
  base_mark_unuse(signature);
  ptr += 8;

  /* Checksum, TODO: Verify checksum */
  ptr += 1;

  /* OEM */
  ptr += 6;

  revision = *(const u8_t *)ptr;
  ptr += 1;
  log_line_format(LOG_LEVEL_INFO, "ACPI revision: %lu", (u64_t)revision);
  kernel_assert(revision == 0);

  rsdt_addr = *(const u32_t *)ptr;
  ptr += 4;

  kernel_assert((ptr - (uptr_t)multi_boot_info) == len);

  byte_sum = 0;
  for (const byte_t *byte = multi_boot_info; (uptr_t)byte < ptr; byte++) {
    byte_sum += *byte;
  }
  kernel_assert((byte_sum & 0xff) == 0); /* Validate check sum */

  _init_rsdt((byte_t *)(u64_t)rsdt_addr);
}
