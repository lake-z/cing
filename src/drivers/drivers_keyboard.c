#include "drivers_keyboard.h"
#include "containers_string.h"
#include "drivers_screen.h"
#include "interrupts.h"
#include "kernel_panic.h"
#include "drivers_port.h"

typedef enum {
  KEYBOARD_CMD_DISABLE_SCAN = 0xad,
  KEYBOARD_CMD_READ_CONTROLLER_RAM_0 = 0x20,
  KEYBOARD_CMD_ACESS_SCAN_CODE = 0xf0
} keyboard_cmd_t;

base_private bo_t _status_data_is_empty(byte_t status_byte)
{
  return !byte_bit_get(status_byte, 0);
}

/*
base_private bo_t _status_cmd_is_empty(byte_t status_byte)
{
  return !byte_bit_get(status_byte, 1);
}

base_private void _wait_cmd_available(void)
{
  byte_t status;

  do {
    status = port_read_byte(PORT_NO_KEYBOARD_CMD);
  } while (!_status_cmd_is_empty(status));
}
*/

base_private void _wait_data_available(void)
{
  byte_t status;

  do {
    status = port_read_byte(PORT_NO_KEYBOARD_CMD);
  } while (_status_data_is_empty(status));
}

/*
base_private void _cmd_send(keyboard_cmd_t cmd)
{
  port_write_byte(PORT_NO_KEYBOARD_CMD, (byte_t)cmd);
}

base_private void _data_write(byte_t data)
{
  port_write_byte(PORT_NO_KEYBOARD_DATA, data);
}
*/

base_private byte_t _data_read(void)
{
  return port_read_byte(PORT_NO_KEYBOARD_DATA);
}

base_private void keyboard_irq_handler(
    intr_id_t id base_may_unuse, intr_parameters_t *para base_may_unuse)
{
  byte_t data;

  _wait_data_available();
  data = _data_read();

  switch (data) {
  case 0x11:
    screen_write_at('W', 0, 0);
    break;
  case 0x15:
    screen_write_at('Y', 0, 0);
    break;
  case 0x19:
    screen_write_at('P', 0, 0);
    break;
  default:
    break;
  }
}

void keyboard_init(void)
{
  intr_handler_register(INTR_ID_IRQ_KEYBOARD, keyboard_irq_handler);
}
