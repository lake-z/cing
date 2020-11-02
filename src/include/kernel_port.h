#ifndef ___KERNEL_PORT
#define ___KERNEL_PORT

#include "base.h"

typedef enum {
  PORT_NO_PIC_MASTER_CMD = 0x20,
  PORT_NO_PIC_MASTER_DATA = 0x21,
  PORT_NO_PIC_SLAVE_CMD = 0xA0,
  PORT_NO_PIC_SLAVE_DATA = 0xa1,
  PORT_NO_KEYBOARD_CMD = 0x64,
  PORT_NO_KEYBOARD_DATA = 0x60,
  PORT_NO_VGA_CMD = 0x3d4,
  PORT_NO_VGA_DATA = 0x3d5,
  PORT_NO_VGA_STATUS_1 = 0x3da,
  PORT_NO_VGA_ATTRIBUTE_WRITE = 0x3c0,
  PORT_NO_VGA_ATTRIBUTE_READ = 0x3c1,
} port_no_t;

void port_write_byte(port_no_t port, byte_t data);
byte_t port_read_byte(port_no_t port);

#endif
