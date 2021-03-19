#include "drivers_pcie.h"
#include "containers_string.h"
#include "kernel_panic.h"
#include "log.h"
#include "mm.h"

/* Currently supports no more than 64 bus groups, it's not limited by PCIe
 * spec. */
#define _GROUP_CAP 8
#define _FUNCTIONS_CAP 128

/* Every PCIe Function is uniquely identified by the Device it resides within
 * and the Bus to which the Device connects. This unique identifier is commonly 
 * referred to as a ‘BDF’.
 */
/* PCIe spec limits no more than 256 bus in one system. The initial Bus Number, 
 * Bus 0, is typically assigned by hardware to the Root Complex. */
base_private u64_t _BUS_MAX = 256;
/* PCIe spec limits up to 32 device per bus. */
base_private u64_t _DEVICE_MAX = 32;
/* PCIe spec limits up to 8 functions per device. */
base_private u64_t _FUNCTION_MAX = 8;

/* PCIe config space for every function can be as long as 4KB, and is divided
 * into 3 sections:
 *  [1] PCI header: 64 bytes
 *  [2] PCI capabilities: 192 bytes
 *  [3] PCIe extended capabilities: Can be as long as 4KB - 256 bytes.
 *
 * First access to a Function after a reset condition must be a Configuration
 * read of both bytes of the Vendor ID. */
base_private uptr_t _CONFIG_SPACE_SIZE = 4096;

/* PCIe spec requires config space to be aligned with 256MB, which equals
 * 256 buses * 32 devices * 8 functions * 4KB. */
base_private uptr_t _CONFIG_SPACE_ALIGN = 256 * 32 * 8 * 4 * 1024;

struct d_pcie_group {
  uptr_t cfg_space_base_padd;
  u16_t group_no;
  u8_t bus_start;
  u8_t bus_end;
  u32_t reserved;
} base_struct_packed;

struct d_pcie_func {
  d_pcie_group_t *group;
  u64_t bus;
  u64_t dev;
  u64_t fun;
  u16_t vendor_id;
  u16_t device_id;
  u8_t header_type;
  /* PCIe function class code is constitued with base class, sub class and
   * programming interface. */
  u8_t base_class;
  u8_t sub_class;
  u8_t pi;
};

base_private d_pcie_group_t _groups[_GROUP_CAP];
base_private ucnt_t _gropp_cnt;
base_private d_pcie_func_t _functions[_FUNCTIONS_CAP];
base_private ucnt_t _func_cnt;

base_private uptr_t _cfg_space_pa(
    d_pcie_group_t *group, u64_t bus, u64_t dev, u64_t fun, u64_t off)
{
  uptr_t pa;
  kernel_assert(
      mm_align_check(group->cfg_space_base_padd, _CONFIG_SPACE_ALIGN));
  kernel_assert(bus < _BUS_MAX);
  kernel_assert(dev < _DEVICE_MAX);
  kernel_assert(fun < _FUNCTION_MAX);
  kernel_assert(off < 4096);

  pa = group->cfg_space_base_padd | bus << 20 | dev << 15 | fun << 12;
  pa |= off;
  return pa;
}

base_private byte_t _cfg_space_read_byte(
    d_pcie_group_t *group, u64_t bus, u64_t dev, u64_t fun, u64_t off)
{
  uptr_t padd;
  kernel_assert(off < _CONFIG_SPACE_SIZE);

  padd = _cfg_space_pa(group, bus, dev, fun, off);
  return *(byte_t *)padd;
}

base_private u16_t _cfg_space_read_word(
    d_pcie_group_t *group, u64_t bus, u64_t dev, u64_t fun, u64_t off)
{
  kernel_assert(off % 2 == 0);
  kernel_assert(off <= (_CONFIG_SPACE_SIZE - 2));
  uptr_t padd = _cfg_space_pa(group, bus, dev, fun, off);
  return *(u16_t *)padd;
}

base_private u32_t _cfg_space_read_dword(
    d_pcie_group_t *group, u64_t bus, u64_t dev, u64_t fun, u64_t off)
{
  kernel_assert(off % 4 == 0);
  kernel_assert(off <= (_CONFIG_SPACE_SIZE - 4));
  uptr_t padd = _cfg_space_pa(group, bus, dev, fun, off);
  return *(u32_t *)padd;
}

base_private void _cfg_space_write_dword(
    d_pcie_group_t *group, u64_t bus, u64_t dev, u64_t fun, u64_t off, u32_t val)
{
  kernel_assert(off % 4 == 0);
  kernel_assert(off <= (4096 - 4));
  uptr_t padd = _cfg_space_pa(group, bus, dev, fun, off);
  *(u32_t *)padd = val;
}

/*
base_private void cfg_space_write_byte(
    group_t *group, u64_t bus, u64_t dev, u64_t fun, u64_t off, byte_t val)
{
  uptr_t padd;
  kernel_assert(off < 4096);

  padd = cfg_space_pa(group, bus, dev, fun, off);
  *(byte_t *)padd = val;
}

base_private u16_t cfg_space_write_word(
    group_t *group, u64_t bus, u64_t dev, u64_t fun, u64_t word, u16_t val)
{
  kernel_assert(word < 2048);
  uptr_t padd = cfg_space_pa(group, bus, dev, fun);
  padd |= word << 1;
  *(u16_t *)padd = val;
}


*/

byte_t d_pcie_cfg_space_read_byte(d_pcie_func_t *fun, u64_t off)
{
  return _cfg_space_read_byte(fun->group, fun->bus, fun->dev, fun->fun, off);
}

u16_t d_pcie_cfg_space_read_word(d_pcie_func_t *fun, u64_t off)
{
  return _cfg_space_read_word(fun->group, fun->bus, fun->dev, fun->fun, off);
}

u32_t d_pcie_cfg_space_read_dword(d_pcie_func_t *fun, u64_t off)
{
  return _cfg_space_read_dword(fun->group, fun->bus, fun->dev, fun->fun, off);
}

void d_pcie_cfg_space_write_dword(d_pcie_func_t *fun, u64_t off, u32_t val)
{
  _cfg_space_write_dword(fun->group, fun->bus, fun->dev, fun->fun, off, val);
}

static const char *device_name_8086(u16_t vendor, u16_t device)
{
  const char *name;

  kernel_assert(vendor == 0x8086);

  switch (device) {
  case 0x1911:
    name = "Xeon Core Processor Gaussian Mixture Model";
    break;
  case 0x2918:
    name = "82801IB (ICH9) LPC Interface Controller";
    break;
  case 0x2922:
    name = "82801IR/IO/IH (ICH9R/DO/DH) 6 port SATA Controller [AHCI mode]";
    break;
  case 0x2930:
    name = "82801I (ICH9 Family) SMBus Controller";
    break;
  case 0x29c0:
    name = "82G33/G31/P35/P31 Express DRAM Controller";
    break;
  case 0x3ea5:
    name = "CoffeeLake-U GT3e [Iris Plus Graphics 655]";
    break;
  case 0x3ed0:
    name = "8th Gen Core Processor Host Bridge/DRAM Registers";
    break;
  case 0x10d3:
    name = "82574L Gigabit Network Connection";
    break;
  case 0x5845:
    name = "QEMU NVM Express Controller";
    break;
  case 0x9de0:
    name = "Cannon Point-LP MEI Controller #1";
    break;
  case 0x9ded:
    name = "Cannon Point-LP USB 3.1 xHCI Controller";
    break;
  case 0x9def:
    name = "Cannon Point-LP Shared SRAM";
    break;
  case 0x9df0:
    name = "Cannon Point-LP CNVi [Wireless-AC]";
    break;
  case 0x9df9:
    name = "Cannon Point-LP Thermal Controller";
    break;
  default:
    log_line_format(LOG_LEVEL_DEBUG, "Unknown device: %lu", (u64_t)device);
    name = "Unknown";
    break;
  }
  return name;
}

static const char *device_name_1234(u16_t vendor, u16_t device)
{
  const char *name;

  kernel_assert(vendor == 0x1234);
  switch (device) {
  case 0x1111:
    name = "QEMU Virtual Video Controller";
    break;
  default:
    log_line_format(LOG_LEVEL_DEBUG, "Unknown device: %lu", (u64_t)device);
    name = "Unknown";
    break;
  }
  return name;
}

static void iden_name(
    u16_t vendor, u16_t device, const char **vname, const char **dname)
{
  *vname = NULL;
  *dname = NULL;

  if (vendor == 0x10EC) {
    *vname = "Realtek Semiconductor Co., Ltd.";
  } else if (vendor == 0x1234) {
    *vname = "Technical Corp";
    *dname = device_name_1234(vendor, device);
  } else if (vendor == 0x15b7) {
    *vname = "Sandisk Corp";
    if (device == 0x5009) {
      *dname = "WD Blue SN550 NVMe SSD";
    }
  } else if (vendor == 0x8086) {
    *vname = "Intel";
    *dname = device_name_8086(vendor, device);
  }

  if (*vname == NULL) {
    *vname = "Unknown vendor";
    log_line_format(LOG_LEVEL_WARN, "Unknown vendor: %lu", (u64_t)vendor);
  }

  if (*dname == NULL) {
    *dname = "Unknown device";
    log_line_format(LOG_LEVEL_WARN, "Unknown vendor: %lu", (u64_t)device);
  }
}

static const char *class_name(u8_t base, u8_t sub, u8_t pi)
{
  const char *name = NULL;

  if (base == 0x00) {
    if (sub == 0x00) {
      name = "Old device";
    } else {
      kernel_panic("TODO");
    }
  } else if (base == 0x01) {
    if (sub == 0x08) {
      if (pi == 0x02) {
        name = "NVM Express (NVMe) I/O controller";
      }
    } else if (sub == 0x06) {
      if (pi == 0x01) {
        name = "Serial ATA controller - AHCI interface";
      }
    }
  } else if (base == 0x02) {
    if (sub == 0x00) {
      if (pi == 0x00) {
        name = "Ethernet controller";
      }
    } else if (sub == 0x80) {
      if (pi == 0x00) {
        name = "Other network controller";
      }
    }
  } else if (base == 0x03) {
    if (sub == 0x00) {
      if (pi == 0x00) {
        name = "VGA-compatible controller";
      }
    }
  } else if (base == 0x04) {
    if (sub == 0x00) {
      if (pi == 0x00) {
        name = "Video device – vendor specific interface";
      }
    } else if (sub == 0x01) {
      if (pi == 0x00) {
        name = "Audio device – vendor specific interface";
      }
    } else if (sub == 0x02) {
      if (pi == 0x00) {
        name = "Computer telephony device – vendor specific interface";
      }
    } else if (sub == 0x03) {
      if (pi == 0x00) {
        name = "High Definition Audio 1.0 compatible";
      } else if (pi == 0x80) {
        name = "High Definition Audio 1.0 compatible with vendor extensions";
      }
    } else if (sub == 0x80) {
      if (pi == 0x00) {
        name = "Other multimedia device – vendor specific interface";
      }
    }
  } else if (base == 0x05) {
    if (sub == 0x00) {
      if (pi == 0x00) {
        name = "RAM";
      }
    }
  } else if (base == 0x06) {
    if (sub == 0x00) {
      if (pi == 0x00) {
        name = "Host bridge";
      }
    } else if (sub == 0x01) {
      if (pi == 0x00) {
        name = "ISA bridge";
      }
    } else if (sub == 0x02) {
    } else if (sub == 0x03) {
    } else if (sub == 0x04) {
      if (pi == 0x00) {
        name = "PCI-to-PCI bridge";
      } else if (pi == 0x01) {
        name = "Subtractive Decode PCI-to-PCI bridge";
      }
    } else if (sub == 0x05) {
    } else if (sub == 0x06) {
    } else if (sub == 0x07) {
    } else if (sub == 0x08) {
    } else if (sub == 0x09) {
    } else if (sub == 0x0A) {
    } else if (sub == 0x0B) {
    } else if (sub == 0x80) {
    }
  } else if (base == 0x07) {
    if (sub == 0x00) {
      if (pi == 0x00) {
        name = "Generic XT-compatible serial controller";
      } else if (pi == 0x01) {
        name = "16450-compatible serial controller";
      } else if (pi == 0x02) {
        name = "16550-compatible serial controller";
      } else if (pi == 0x03) {
        name = "16650-compatible serial controller";
      } else if (pi == 0x04) {
        name = "16750-compatible serial controller";
      } else if (pi == 0x05) {
        name = "16850-compatible serial controller";
      } else if (pi == 0x06) {
        name = "16950-compatible serial controller";
      }
    } else if (sub == 0x01) {
    } else if (sub == 0x02) {
    } else if (sub == 0x03) {
    } else if (sub == 0x04) {
    } else if (sub == 0x05) {
    } else if (sub == 0x80) {
      if (pi == 0x00) {
        name = "Other communications device";
      }
    }
  } else if (base == 0x08) {
    if (sub == 0x80) {
      if (pi == 0x00) {
        name = "Other system peripheral";
      }
    }
  } else if (base == 0x0C) {
    if (sub == 0x00) {
    } else if (sub == 0x01) {
    } else if (sub == 0x02) {
    } else if (sub == 0x03) {
      if (pi == 0x30) {
        name = "USB Controller following Intel xHCI specification";
      }
    } else if (sub == 0x04) {
    } else if (sub == 0x05) {
      if (pi == 0x00) {
        name = "System Management Bus";
      }
    } else if (sub == 0x06) {
    } else if (sub == 0x07) {
    } else if (sub == 0x08) {
    } else if (sub == 0x09) {
    } else if (sub == 0x0A) {
    } else if (sub == 0x80) {
      if (pi == 0x00) {
        name = "Other Serial Bus Controllers";
      }
    }
  } else if (base == 0x11) {
    if (sub == 0x80) {
      if (pi == 0x00) {
        name = "Other data acquisition/signal processing controllers";
      }
    }
  } else if (base == 0xff) {
    name = "Device does not fit in any defined classes";
  }

  if (name == NULL) {
    log_line_format(LOG_LEVEL_WARN, "Unknown class code: %lu,%lu,%lu",
        (u64_t)base, (u64_t)sub, (u64_t)pi);
    name = "Unknown class";
  }

  return name;
}

ucnt_t d_pcie_get_func_cnt(void)
{
  kernel_assert_d(_func_cnt < _FUNCTIONS_CAP);
  return _func_cnt;
}

d_pcie_func_t *d_pcie_get_func(ucnt_t idx)
{
  return &_functions[idx];
}

u8_t d_pcie_func_get_base_class(d_pcie_func_t *fun)
{
  return fun->base_class;
}

u8_t d_pcie_func_get_sub_class(d_pcie_func_t *fun)
{
  return fun->sub_class;
}

const char *d_pcie_func_get_vendor_name(d_pcie_func_t *fun)
{
  const char *vname;
  const char *dname;
  iden_name(fun->vendor_id, fun->device_id, &vname, &dname);
  return vname;
}

const char *d_pcie_func_get_device_name(d_pcie_func_t *fun)
{
  const char *vname;
  const char *dname;
  iden_name(fun->vendor_id, fun->device_id, &vname, &dname);
  return dname;
}

static void bootstrap_fun(
    d_pcie_group_t *group, u64_t bus, u64_t dev, u64_t fun)
{
  u16_t vendor = _cfg_space_read_word(group, bus, dev, fun, 0);

  /* vender == 0xFFFF means device not present. */
  if (vendor != 0xFFFF) {
    d_pcie_func_t *f;
    const char *vname;
    const char *dname;
    const char *cname;

    f = &_functions[_func_cnt++];
    f->group = group;
    f->bus = bus;
    f->dev = dev;
    f->fun = fun;
    f->vendor_id = vendor;
    f->device_id = _cfg_space_read_word(group, bus, dev, fun, 2);
    f->header_type = _cfg_space_read_byte(group, bus, dev, fun, 0x0E);
    f->base_class = _cfg_space_read_byte(group, bus, dev, fun, 11);
    f->sub_class = _cfg_space_read_byte(group, bus, dev, fun, 10);
    f->pi = _cfg_space_read_byte(group, bus, dev, fun, 9);

    iden_name(f->vendor_id, f->device_id, &vname, &dname);
    cname = class_name(f->base_class, f->sub_class, f->pi);

    log_line_format(LOG_LEVEL_INFO, "[%lu,%lu,%lu]: %s, %s", (u64_t)bus,
        (u64_t)dev, (u64_t)fun, cname, dname);
    log_line_format(LOG_LEVEL_INFO, "%s, header_type: %lu, multi_fun: %lu",
        vname, (u64_t)f->header_type, (u64_t)byte_bit_get(f->header_type, 7));

    kernel_assert(cname != NULL);

    (void)(_cfg_space_read_dword);
  }
}

static void bootstrap_dev(d_pcie_group_t *group, u64_t bus, u64_t dev)
{
  u16_t vendor = _cfg_space_read_word(group, bus, dev, 0, 0);

  /* vender == 0xFFFF means device not present. */
  if (vendor != 0xFFFF) {
    byte_t header_type = _cfg_space_read_byte(group, bus, dev, 0, 0x0E);
    u64_t fun_cnt;
    if (byte_bit_get(header_type, 7)) {
      fun_cnt = 8;
    } else {
      fun_cnt = 1;
    }

    for (u64_t fun = 0; fun < fun_cnt; fun++) {
      bootstrap_fun(group, bus, dev, fun);
    }
  }
}

void d_pcie_bootstrap(const byte_t *mcfg, usz_t len)
{
  kernel_assert(len >= sizeof(d_pcie_group_t));
  kernel_assert((len % sizeof(d_pcie_group_t)) == 0);
  kernel_assert(mcfg != NULL);

  _gropp_cnt = len / sizeof(d_pcie_group_t);
  kernel_assert(_gropp_cnt <= _GROUP_CAP);

  log_line_format(LOG_LEVEL_INFO, "PCIe init, MCFG len: %lu, group count: %lu",
      len, _gropp_cnt);

  _func_cnt = 0;
  for (ucnt_t i = 0; i < _gropp_cnt; i++) {
    _groups[i] = *(d_pcie_group_t *)(mcfg + i * sizeof(d_pcie_group_t));

    kernel_assert(
        mm_align_check(_groups[i].cfg_space_base_padd, _CONFIG_SPACE_ALIGN));

    for (u64_t bus = _groups[i].bus_start; bus < _groups[i].bus_end; bus++) {
      for (u64_t dev = 0; dev < 32; dev++) {
        bootstrap_dev(&_groups[i], bus, dev);
      }
    }
  }
}
