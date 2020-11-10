#include "kernel_port.h"
#include "drivers_serial.h"
#include "containers_string.h"

base_private const u16_t _SPEED_115200_BAUDS = 1;
base_private const u16_t _SPEED_57600_BAUDS = 2;
base_private const u16_t _SPEED_38400_BAUDS = 3;

/*
 * ****************************************************************************
 * All the I/O ports are calculated relative to the data port. This is because
 * all serial ports (COM1, COM2, COM3, COM4) have their ports in the same
 * order, but they start at different values.
 * ****************************************************************************
 */
base_private port_no_t _port_data(port_no_t base)
{
  return base;
}

base_private port_no_t _port_fifo_cmd(port_no_t base)
{
  return base + 2;
}

base_private port_no_t _port_line_cmd(port_no_t base)
{
  return base + 3;
}

base_private port_no_t _port_modem_cmd(port_no_t base)
{
  return base + 4;
}

base_private port_no_t _port_line_status(port_no_t base)
{
  return base + 5;
}

void serial_init(void)
{
  /* Enable DLAB */
  port_write_byte(_port_line_cmd(PORT_NO_SERIAL_COM1), 0x80);

  port_write_byte(_port_data(PORT_NO_SERIAL_COM1), byte_get(_SPEED_115200_BAUDS, 1));
  port_write_byte(_port_data(PORT_NO_SERIAL_COM1), byte_get(_SPEED_115200_BAUDS, 0));

  // cf. https://littleosbook.github.io/#configuring-the-serial-port
  // Bit:     | 7 | 6 | 5 4 3 | 2 | 1 0 |
  // Content: | d | b | prty  | s | dl  |
  // Value:   | 0 | 0 | 0 0 0 | 0 | 1 1 | = 0x03
  port_write_byte(_port_line_cmd(PORT_NO_SERIAL_COM1), 0x03);

  // Enable FIFO, clear them, with 14b threshold
  port_write_byte(_port_fifo_cmd(PORT_NO_SERIAL_COM1), 0xc7);

  // IRQs enabled, RTS/DSR set
  port_write_byte(_port_modem_cmd(PORT_NO_SERIAL_COM1), 0x0B);
}

base_private bo_t _fifo_transmit_is_empty(void)
{
  /* 0x20 = 0010 0000 */
  byte_t data = port_read_byte(_port_line_status(PORT_NO_SERIAL_COM1));
  return (data & 0x20);
}

base_private void _write_char(ch_t c)                                         
{                                                                               
  while (_fifo_transmit_is_empty() == false);
  port_write_byte(PORT_NO_SERIAL_COM1, (byte_t)c);
}   

void serial_write_str(const ch_t* str, usz_t len)                                
{                                                                               
  for (usz_t i = 0; i < len; i++) {                                    
    _write_char(str[i]);                                                  
  }                                                                             
}  
