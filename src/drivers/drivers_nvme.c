/* NVMe driver. */

#include "drivers_nvme.h"
#include "drivers_pcie.h"
#include "log.h"

base_private const u8_t _CLASS_CODE_BASE = 0x01;
base_private const u8_t _CLASS_CODE_SUB = 0x08;

typedef struct d_nvme_ctrl d_nvme_ctrl_t;
struct d_nvme_ctrl
{
  d_pcie_func_t *pcie;
};

/*
void d_nvme_add_fun(d_pcie_func_t *pcie_fun)
{
  u32_t bar0;
  byte_t by;

  cfg_space_write_dword(group, bus, dev, fun, 0x10, 0xFFFFFFFF);
  bar0 = cfg_space_read_dword(group, bus, dev, fun, 0x10);
  by = bar0 & 0xFF;

  log_line_start(LOG_LEVEL_DEBUG);
  log_str(LOG_LEVEL_DEBUG, "bar0: ");
  log_uint(LOG_LEVEL_DEBUG, bar0);
  log_str(LOG_LEVEL_DEBUG, "by: ");
  log_uint(LOG_LEVEL_DEBUG, by);
  log_line_end(LOG_LEVEL_DEBUG);

  // Only supports non-prefectchable, and 64-bit address space 
  kernel_assert(by == 4);

}
*/

void d_nvme_bootstrap(void)
{
  ucnt_t func_cnt = d_pcie_get_func_cnt();
  ucnt_t nvme_cnt;
  d_pcie_func_t *fun;

  nvme_cnt = 0;
  for (ucnt_t i = 0; i < func_cnt; i++) {
    fun = d_pcie_get_func(i);
    if (d_pcie_func_get_base_class(fun) == _CLASS_CODE_BASE && d_pcie_func_get_sub_class(fun) == _CLASS_CODE_SUB) {
      log_line_start(LOG_LEVEL_DEBUG);
      log_str(LOG_LEVEL_DEBUG, "Found NVMe function: ");
      log_str(LOG_LEVEL_DEBUG, d_pcie_func_get_vendor_name(fun));
      log_str(LOG_LEVEL_DEBUG, ", ");
      log_str(LOG_LEVEL_DEBUG, d_pcie_func_get_device_name(fun));
      log_line_end(LOG_LEVEL_DEBUG);
      nvme_cnt++;
    }
  }
}
