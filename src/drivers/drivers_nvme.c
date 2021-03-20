/* NVMe driver. */

#include "drivers_nvme.h"
#include "drivers_pcie.h"
#include "kernel_panic.h"
#include "log.h"
#include "mm.h"

base_private const u8_t _CLASS_CODE_BASE = 0x01;
base_private const u8_t _CLASS_CODE_SUB = 0x08;
base_private const u64_t _CFG_SPACE_BAR_0 = 0x10;
/* Masks bit 14 ~ 31 */
base_private const u32_t _CFG_SPACE_BAR_0_BASE_MASK = 0xFFFFC000;

typedef struct d_nvme_ctrl d_nvme_ctrl_t;
struct d_nvme_ctrl {
  mm_allocator_t *mm;
  d_pcie_func_t *pcie;
  usz_t reg_size;
  byte_t *regs;
};

#define _CTRL_CAP 32
base_private d_nvme_ctrl_t _ctrls[_CTRL_CAP];
base_private ucnt_t _ctrl_cnt;

base_private void _ctrl_add(d_pcie_func_t *pcie_fun)
{
  d_nvme_ctrl_t *ctrl;
  u32_t bar0;
  byte_t by;

  ctrl = &_ctrls[_ctrl_cnt++];
  ctrl->mm = mm_allocator_new();
  ctrl->pcie = pcie_fun;

  d_pcie_cfg_space_write_dword(ctrl->pcie, _CFG_SPACE_BAR_0, 0xFFFFFFFF);
  bar0 = d_pcie_cfg_space_read_dword(ctrl->pcie, _CFG_SPACE_BAR_0);
  by = bar0 & 0xFF;
  /* Only supports non-prefectchable, and 64-bit address space */
  kernel_assert(by == 4);

  bar0 = bar0 & _CFG_SPACE_BAR_0_BASE_MASK;
  ctrl->reg_size = mm_align_class((u64_t)bar0);
  log_line_format(LOG_LEVEL_INFO, "bar0 size: %lu", ctrl->reg_size);
}

void d_nvme_bootstrap(void)
{
  ucnt_t func_cnt = d_pcie_get_func_cnt();
  d_pcie_func_t *fun;

  for (ucnt_t i = 0; i < func_cnt; i++) {
    fun = d_pcie_get_func(i);
    if (d_pcie_func_get_base_class(fun) == _CLASS_CODE_BASE &&
        d_pcie_func_get_sub_class(fun) == _CLASS_CODE_SUB) {
      _ctrl_add(fun);
      log_line_format(LOG_LEVEL_DEBUG, "NVMe func found: %s, %s",
          d_pcie_func_get_vendor_name(fun), d_pcie_func_get_device_name(fun));
    }
  }
}
