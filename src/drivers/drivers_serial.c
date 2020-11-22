#include "drivers_serial.h"
#include "containers_string.h"
#include "drivers_port.h"

base_private const u16_t _SPEED_115200_BAUDS = 1;
base_private const u16_t _SPEED_57600_BAUDS = 2;
base_private const u16_t _SPEED_38400_BAUDS = 3;

void serial_init(void)
{
  /* Enable DLAB */
  port_write_byte(PORT_NO_SERIAL_COM1_CMD_LINE, 0x80);

  port_write_byte(PORT_NO_SERIAL_COM1_DATA, byte_get(_SPEED_115200_BAUDS, 1));
  port_write_byte(PORT_NO_SERIAL_COM1_DATA, byte_get(_SPEED_115200_BAUDS, 0));

  // cf. https://littleosbook.github.io/#configuring-the-serial-port
  // Bit:     | 7 | 6 | 5 4 3 | 2 | 1 0 |
  // Content: | d | b | prty  | s | dl  |
  // Value:   | 0 | 0 | 0 0 0 | 0 | 1 1 | = 0x03
  port_write_byte(PORT_NO_SERIAL_COM1_CMD_LINE, 0x03);

  // Enable FIFO, clear them, with 14b threshold
  port_write_byte(PORT_NO_SERIAL_COM1_CMD_FIFO, 0xc7);

  // IRQs enabled, RTS/DSR set
  port_write_byte(PORT_NO_SERIAL_COM1_CMD_MODEM, 0x0B);
}

base_private bo_t _fifo_transmit_is_empty(void)
{
  /* 0x20 = 0010 0000 */
  byte_t data = port_read_byte(PORT_NO_SERIAL_COM1_STATUS_LINE);
  return (data & 0x20);
}

base_private void _write_char(ch_t c)
{
  while (_fifo_transmit_is_empty() == false)
    ;
  port_write_byte(PORT_NO_SERIAL_COM1, (byte_t)c);
}

void serial_write_str(const ch_t *str, usz_t len)
{
  for (usz_t i = 0; i < len; i++) {
    _write_char(str[i]);
  }
}
