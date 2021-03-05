#include "drivers_pcie.h"
#include "containers_string.h"
#include "kernel_panic.h"
#include "log.h"
#include "mm.h"

/* Every PCIe Function is uniquely identified by the Device it resides within
 * and the Bus to which the Device connects. This unique identifier is commonly 
 * referred to as a ‘BDF’.
 *
 * BUS: Up to 256 Bus Numbers can be assigned by configuration software. The
 * initial Bus Number, Bus 0, is typically assigned by hardware to the Root
 * Complex. Bus 0 consists of a Virtual PCI bus with integrated endpoints and
 * Virtual PCI‐to‐PCI Bridges which are hard‐coded with a Device number and 
 * Function number. 
 * DEVICE: Up to 32 devices per bus.
 * FUNCTION: Up to 8 functions per device.
 */

/* PCIe config space is divided into 3 sections:
 *  [1] PCI header: 64 bytes
 *  [2] PCI capabilities: 192 bytes
 *  [3] PCIe extended capabilities: Can be as long as 4KB - 256 bytes */

/* First access to a Function after a reset condition must be a Configuration 
 * read of both bytes of the Vendor ID. */

/* PCIe spec requires config space to be aligned with 256MB. */
base_private uptr_t _CONFIG_SPACE_ALIGN = 256 * 1024 * 1024;

/* Currently supports no more than 64 bus groups.*/
#define _GROUP_MAX 64

typedef struct bus_group {
  uptr_t cfg_space_base_padd;
  u16_t group_no;
  u8_t bus_start;
  u8_t bus_end;
  u32_t reserved;
} base_struct_packed group_t;

base_private group_t _groups[_GROUP_MAX];
base_private ucnt_t _gropp_cnt;

base_private uptr_t cfg_space_pa(
    group_t *group, u64_t bus, u64_t dev, u64_t fun)
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

  padd = cfg_space_pa(group, bus, dev, fun);
  padd |= byte;
  return *(byte_t *)padd;
}

base_private u16_t cfg_space_read_word(
    group_t *group, u64_t bus, u64_t dev, u64_t fun, u64_t word)
{
  kernel_assert(word < 2048);
  uptr_t padd = cfg_space_pa(group, bus, dev, fun);
  padd |= word << 1;
  return *(u16_t *)padd;
}

static const char *device_name_8086(u16_t vendor, u16_t device)
{
  const char *name;

  kernel_assert(vendor == 0x8086);

  switch (device) {
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
  case 0x10d3:
    name = "82574L Gigabit Network Connection";
    break;
  case 0x5845:
    name = "QEMU NVM Express Controller";
    break;
  default:
    log_line_start(LOG_LEVEL_DEBUG);
    log_str(LOG_LEVEL_INFO, "Unknown device: ");
    log_uint(LOG_LEVEL_INFO, device);
    log_line_end(LOG_LEVEL_DEBUG);
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
    log_line_start(LOG_LEVEL_DEBUG);
    log_str(LOG_LEVEL_INFO, "Unknown device: ");
    log_uint(LOG_LEVEL_INFO, device);
    log_line_end(LOG_LEVEL_DEBUG);
    name = "Unknown";
    break;
  }
  return name;
}

static void iden_name(
    u16_t vendor, u16_t device, const char **vname, const char **dname)
{
  switch (vendor) {
  case 0x8086:
    *vname = "Intel";
    *dname = device_name_8086(vendor, device);
    break;
  case 0x1234:
    *vname = "Technical Corp";
    *dname = device_name_1234(vendor, device);
    break;
  default:
    *vname = "Unknown";
    *dname = "Unknown";
    log_line_start(LOG_LEVEL_DEBUG);
    log_str(LOG_LEVEL_INFO, "Unknown vendor: ");
    log_uint(LOG_LEVEL_INFO, vendor);
    log_line_end(LOG_LEVEL_DEBUG);
    break;
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
    }
  } else if (base == 0x03) {
    if (sub == 0x00) {
      if (pi == 0x00) {
        name = "VGA-compatible controller";
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
    }
  } else if (base == 0x0C) {
    if (sub == 0x05) {
      if (pi == 0x00) {
        name = "System Management Bus";
      }
    }
  }

  (void)(pi);
  if (name == NULL) {
    static const usz_t msg_len = 256;
    char msg[msg_len];
    const char *msg_part = "Unknown class: ";
    usz_t off =
        str_buf_marshal_str(msg, 0, msg_len, msg_part, str_len(msg_part));
    off += str_buf_marshal_uint(msg, off, msg_len, base);
    msg_part = ", ";
    off += str_buf_marshal_str(msg, off, msg_len, msg_part, str_len(msg_part));
    off += str_buf_marshal_uint(msg, off, msg_len, sub);
    msg_part = ", ";
    off += str_buf_marshal_str(msg, off, msg_len, msg_part, str_len(msg_part));
    off += str_buf_marshal_uint(msg, off, msg_len, pi);
    off += str_buf_marshal_terminator(msg, off, msg_len);
    kernel_panic(msg);
  }

  return name;
}

static void bootstrap_fun(group_t *group, u64_t bus, u64_t dev, u64_t fun)
{
  u16_t vendor = cfg_space_read_word(group, bus, dev, fun, 0);

  /* vender == 0xFFFF means device not present. */
  if (vendor != 0xFFFF) {
    const char *vname;
    const char *dname;
    const char *cname;
    u16_t device = cfg_space_read_word(group, bus, dev, fun, 1);
    byte_t header_type = cfg_space_read_byte(group, bus, dev, fun, 0x0E);
    u8_t base_class = cfg_space_read_byte(group, bus, dev, fun, 11);
    u8_t sub_class = cfg_space_read_byte(group, bus, dev, fun, 10);
    u8_t programming_interface = cfg_space_read_byte(group, bus, dev, fun, 9);

    iden_name(vendor, device, &vname, &dname);
    cname = class_name(base_class, sub_class, programming_interface);

    log_line_start(LOG_LEVEL_INFO);
    log_str(LOG_LEVEL_INFO, "[");
    log_uint(LOG_LEVEL_INFO, bus);
    log_str(LOG_LEVEL_INFO, ",");
    log_uint(LOG_LEVEL_INFO, dev);
    log_str(LOG_LEVEL_INFO, ",");
    log_uint(LOG_LEVEL_INFO, fun);
    log_str(LOG_LEVEL_INFO, "]: ");
    log_str(LOG_LEVEL_INFO, vname);
    log_str(LOG_LEVEL_INFO, ", ");
    log_str(LOG_LEVEL_INFO, dname);
    log_line_end(LOG_LEVEL_INFO);

    log_line_start(LOG_LEVEL_INFO);
    log_str(LOG_LEVEL_INFO, "header_type: ");
    log_uint(LOG_LEVEL_INFO, header_type);
    log_str(LOG_LEVEL_INFO, ", multi_fun: ");
    log_uint(LOG_LEVEL_INFO, byte_bit_get(header_type, 7));
    log_str(LOG_LEVEL_INFO, ", class: ");
    log_str(LOG_LEVEL_INFO, cname);
    log_line_end(LOG_LEVEL_INFO);
  }
}

static void bootstrap_dev(group_t *group, u64_t bus, u64_t dev)
{
  u16_t vendor = cfg_space_read_word(group, bus, dev, 0, 0);

  /* vender == 0xFFFF means device not present. */
  if (vendor != 0xFFFF) {
    byte_t header_type = cfg_space_read_byte(group, bus, dev, 0, 0x0E);
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
  kernel_assert(len >= sizeof(group_t));
  kernel_assert((len % sizeof(group_t)) == 0);
  kernel_assert(mcfg != NULL);

  _gropp_cnt = len / sizeof(group_t);
  kernel_assert(_gropp_cnt <= _GROUP_MAX);

  log_line_start(LOG_LEVEL_INFO);
  log_str(LOG_LEVEL_INFO, "PCIe init, MCFG len: ");
  log_uint(LOG_LEVEL_INFO, len);
  log_str(LOG_LEVEL_INFO, ", group count: ");
  log_uint(LOG_LEVEL_INFO, _gropp_cnt);
  log_line_end(LOG_LEVEL_INFO);

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

    for (u64_t bus = _groups[i].bus_start; bus < _groups[i].bus_end; bus++) {
      for (u64_t dev = 0; dev < 32; dev++) {
        bootstrap_dev(&_groups[i], bus, dev);
      }
    }
  }
}
