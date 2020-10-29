#include "kernel_port.h"

void port_write_byte(port_no_t port, byte_t data)
{
  u16_t no = (u16_t)port;
  __asm__("out %%al, %%dx" : : "a"(data), "d"(no));
}

byte_t port_read_byte(port_no_t port)
{
  byte_t result;
  __asm__("in %%dx, %%al" : "=a"(result) : "d"(port));
  return result;
}
