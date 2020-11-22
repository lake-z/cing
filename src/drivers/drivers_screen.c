#include "drivers_screen.h"
#include "containers_string.h"
#include "drivers_port.h"
#include "kernel_panic.h"

base_private byte_t *const _video_buffer = (byte_t *)0xB8000;
base_private const byte_t _CHAR_NULL = 0;

void screen_init()
{
  screen_cursor_disable();
  screen_blink_disable();
}

void screen_cursor_disable(void)
{
  port_write_byte(PORT_NO_VGA_CMD, 0x0a);
  port_write_byte(PORT_NO_VGA_DATA, 0x20);
}

void screen_blink_disable(void)
{
  byte_t data;

  /* Read to reset index/data flip-flop */
  port_read_byte(PORT_NO_VGA_STATUS_1);

  /* Set register index to 0x30 */
  port_write_byte(PORT_NO_VGA_ATTRIBUTE_WRITE, 0x30);

  /* Read register contents than cleat bit 3 to disable blink */
  data = port_read_byte(PORT_NO_VGA_ATTRIBUTE_READ);
  data = byte_bit_clear(data, 3);
  port_write_byte(PORT_NO_VGA_ATTRIBUTE_WRITE, data);
}

base_private usz_t _offset_max(void)
{
  return (2 * SCREEN_WIDTH * SCREEN_HEIGHT);
}

base_private usz_t _offset_of_char(usz_t row, usz_t col)
{
  kernel_assert(row < SCREEN_HEIGHT);
  kernel_assert(col < SCREEN_WIDTH);
  usz_t offset = 2 * (row * SCREEN_WIDTH + col);
  kernel_assert((offset + 1) < _offset_max());
  return offset;
}

base_private usz_t _offset_of_attr(usz_t row, usz_t col)
{
  return _offset_of_char(row, col) + 1;
}

void screen_set_attr_at(
    screen_color_t fg, screen_color_t bg, usz_t row, usz_t col)
{
  u8_t fg_c = (u8_t)(fg);
  u8_t bg_c = (u8_t)(bg);
  u8_t cc = fg_c | (u8_t)(bg_c << 4);
  usz_t offset = _offset_of_attr(row, col);
  _video_buffer[offset] = cc;
}

void screen_set_fg_at(screen_color_t fg, usz_t row, usz_t col)
{
  byte_t fg_c = (u8_t)(fg);
  usz_t offset = _offset_of_attr(row, col);
  byte_t attr = _video_buffer[offset];

  attr = (byte_t)((attr & 0xf0) | fg_c);
  _video_buffer[offset] = attr;
}

void screen_set_bg_at(screen_color_t bg, usz_t row, usz_t col)
{
  byte_t cc = (u8_t)(bg);
  usz_t offset = _offset_of_attr(row, col);
  byte_t attr = _video_buffer[offset];

  attr = (byte_t)((attr & 0x0f) | (cc << 4));
  _video_buffer[offset] = attr;
}

void screen_write_at(byte_t c, usz_t row, usz_t col)
{
  usz_t offset;

  offset = _offset_of_char(row, col);
  _video_buffer[offset] = c;
}

void screen_clean_at(screen_color_t bg, usz_t row, usz_t col)
{
  screen_write_at(_CHAR_NULL, row, col);
  screen_set_bg_at(bg, row, col);
}

void screen_box_thin_border_v_at(usz_t row, usz_t col)
{
  screen_write_at((byte_t)179, row, col);
}

void screen_box_thin_border_h_at(usz_t row, usz_t col)
{
  screen_write_at((byte_t)196, row, col);
}

void screen_box_thin_corner_ne_at(usz_t row, usz_t col)
{
  screen_write_at((byte_t)191, row, col);
}

/* Draw a south west corner of thin box at given coordination */
void screen_box_thin_corner_sw_at(usz_t row, usz_t col)
{
  screen_write_at((byte_t)192, row, col);
}

void screen_box_thin_corner_se_at(usz_t row, usz_t col)
{
  screen_write_at((byte_t)217, row, col);
}

void screen_box_thin_corner_nw_at(usz_t row, usz_t col)
{
  screen_write_at((byte_t)218, row, col);
}

void screen_clean(screen_color_t bg)
{
  for (usz_t row = 0; row < SCREEN_HEIGHT; row++) {
    for (usz_t col = 0; col < SCREEN_WIDTH; col++) {
      screen_clean_at(bg, row, col);
    }
  }
}

/* ************************************************************************* * 
   Following APIs are deprecated, clean them up when it's feasible
 * ************************************************************************* */
void screen_write_str(const char *str,
    screen_color_t fg base_may_unuse,
    screen_color_t bg base_may_unuse,
    usz_t row,
    usz_t col)
{
  for (usz_t i = 0; (i + col) < SCREEN_WIDTH; i++) {
    if (str[i] == '\0') {
      break;
    }
    screen_write_at((byte_t)str[i], row, i + col);
  }
}

void screen_write_uint(
    u64_t val, screen_color_t fg, screen_color_t bg, usz_t row, usz_t col)
{
  base_private const usz_t _MSG_LEN = 128;
  ch_t msg[_MSG_LEN];
  usz_t msg_ptr;

  msg_ptr = str_buf_marshal_uint(msg, 0, _MSG_LEN, val);
  msg[msg_ptr] = '\0';

  screen_write_str(msg, fg, bg, row, col);
}

void screen_write_byte_hex(byte_t *bytes,
    usz_t len,
    screen_color_t fg,
    screen_color_t bg,
    usz_t row,
    usz_t col)
{
  ch_t msg[SCREEN_WIDTH];
  usz_t msg_len = 0;
  msg_len +=
      str_buf_marshal_bytes_in_hex(msg, msg_len, SCREEN_WIDTH, bytes, len);
  msg_len += str_buf_marshal_terminator(msg, msg_len, SCREEN_WIDTH);
  screen_write_str(msg, fg, bg, row, col);
}
