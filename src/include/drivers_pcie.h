#ifndef ___DRIVERS_PCIE
#define ___DRIVERS_PCIE

#include "base.h"

typedef struct d_pcie_group d_pcie_group_t;
typedef struct d_pcie_func d_pcie_func_t;

void d_pcie_bootstrap(const byte_t *mcfg, usz_t len);
ucnt_t d_pcie_get_func_cnt(void);
d_pcie_func_t *d_pcie_get_func(ucnt_t idx);

u8_t d_pcie_func_get_base_class(d_pcie_func_t *fun);
u8_t d_pcie_func_get_sub_class(d_pcie_func_t *fun);
const char *d_pcie_func_get_vendor_name(d_pcie_func_t *fun);
const char *d_pcie_func_get_device_name(d_pcie_func_t *fun);

byte_t d_pcie_cfg_space_read_byte(d_pcie_func_t *fun, u64_t off);
u16_t d_pcie_cfg_space_read_word(d_pcie_func_t *fun, u64_t off);
u32_t d_pcie_cfg_space_read_dword(d_pcie_func_t *fun, u64_t off);
void d_pcie_cfg_space_write_dword(d_pcie_func_t *fun, u64_t off, u32_t val);

#endif
