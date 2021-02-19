#include "drivers_pcie.h"
#include "kernel_panic.h"
#include "log.h"
#include "mm.h"

#define _GROUP_MAX 64
base_private uptr_t _CONFIG_SPACE_ALIGN = 256 * 1024 * 1024;

typedef struct bus_group {
  uptr_t cfg_space_base_padd;
  u16_t group_no;
  u8_t bus_start;
  u8_t bus_end;
  u32_t reserved;
} base_struct_packed group_t;

base_private group_t _groups[_GROUP_MAX];
base_private ucnt_t _gropp_cnt;

base_private uptr_t cfg_space_padd(group_t *group, u64_t bus, u64_t dev, u64_t fun)
{
  kernel_assert(
      mm_align_check(group->cfg_space_base_padd, _CONFIG_SPACE_ALIGN));
  kernel_assert(bus < 256);
  kernel_assert(dev < 32);
  kernel_assert(fun < 8);
  uptr_t pa = group->cfg_space_base_padd | bus << 20 | dev << 15 | fun << 12;
  return pa;
}

base_private byte_t cfg_space_read_byte(
    group_t *group, u64_t bus, u64_t dev, u64_t fun, u64_t byte)
{
  uptr_t padd;
  kernel_assert(byte < 4096);

  padd = cfg_space_padd(group, bus, dev, fun);
  padd |= byte;
  return *(byte_t *)padd;
}

base_private u16_t cfg_space_read_word(
    group_t *group, u64_t bus, u64_t dev, u64_t fun, u64_t word)
{
  kernel_assert(word < 2048);
  uptr_t padd = cfg_space_padd(group, bus, dev, fun);
  padd |= word << 1;
  return *(u16_t *)padd;
}

void pcie_bootstrap(const byte_t *mcfg, usz_t len)
{
  log_line_start(LOG_LEVEL_INFO);
  log_str(LOG_LEVEL_INFO, "PCIe init, MCFG len: ");
  log_uint(LOG_LEVEL_INFO, len);
  log_line_end(LOG_LEVEL_INFO);

  kernel_assert(len >= sizeof(group_t));
  kernel_assert((len % sizeof(group_t)) == 0);
  kernel_assert(mcfg != NULL);

  _gropp_cnt = len / sizeof(group_t);
  kernel_assert(_gropp_cnt <= _GROUP_MAX);
  for (ucnt_t i = 0; i < _gropp_cnt; i++) {
    _groups[i] = *(group_t *)(mcfg + i * sizeof(group_t));

    log_line_start(LOG_LEVEL_INFO);
    log_str(LOG_LEVEL_INFO, "PCIe group ");
    log_uint(LOG_LEVEL_INFO, _groups[i].group_no);
    log_str(LOG_LEVEL_INFO, " cfg space: ");
    log_uint_of_size(LOG_LEVEL_INFO, _groups[i].cfg_space_base_padd);
    kernel_assert(
        mm_align_check(_groups[i].cfg_space_base_padd, _CONFIG_SPACE_ALIGN));
    log_line_end(LOG_LEVEL_INFO);

    for (u64_t dev = 0; dev < 32; dev++) {
      u16_t vendor = cfg_space_read_word(&_groups[0], 0, dev, 0, 0);
      if (vendor != 0xFFFF) {
        u16_t device = cfg_space_read_word(&_groups[0], 0, dev, 0, 1);
        byte_t header_type = cfg_space_read_byte(&_groups[0], 0, dev, 0, 0x0E);

        log_line_start(LOG_LEVEL_INFO);
        log_str(LOG_LEVEL_INFO, "vendor: ");
        log_uint(LOG_LEVEL_INFO, vendor);
        log_str(LOG_LEVEL_INFO, ", device: ");
        log_uint(LOG_LEVEL_INFO, device);
        log_str(LOG_LEVEL_INFO, ", header_type: ");
        log_uint(LOG_LEVEL_INFO, header_type);
        log_line_end(LOG_LEVEL_INFO);
      }
    }
  }
}
