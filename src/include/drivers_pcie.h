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

#endif
