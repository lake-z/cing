#include "drivers_pcie.h"
#include "kernel_panic.h"
#include "log.h"

typedef struct bus_group {
  uptr_t cfg_space_base_padd;
  u16_t group_no;
  u8_t bus_start;
  u8_t bus_end;
  u32_t reserved;
} base_struct_packed bus_group_t;

void pcie_init(const byte_t *mcfg, usz_t len)
{
  bus_group_t *group;

  log_line_start(LOG_LEVEL_INFO);
  log_str(LOG_LEVEL_INFO, "PCIe init, MCFG len: ");
  log_uint(LOG_LEVEL_INFO, len);
  log_line_end(LOG_LEVEL_INFO);

  kernel_assert(len >= sizeof(bus_group_t));
  kernel_assert((len % sizeof(bus_group_t)) == 0);
  kernel_assert(mcfg != NULL);
  group = (bus_group_t *)mcfg;

  log_line_start(LOG_LEVEL_INFO);
  log_str(LOG_LEVEL_INFO, "PCIe group ");
  log_uint(LOG_LEVEL_INFO, group->group_no);
  log_str(LOG_LEVEL_INFO, " cfg space: ");
  log_uint_of_size(LOG_LEVEL_INFO, group->cfg_space_base_padd);
  log_line_end(LOG_LEVEL_INFO);
}
