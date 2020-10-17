#include "kernel_panic.h"
#include "drivers_screen.h"
#include "containers_string.h"

base_private ch_t *_video_buffer;
base_private usz_t _col_ptr;
base_private usz_t _row_ptr;

base_private ch_t _color_code(
  screen_color_t fg,
  screen_color_t bg
)
{
  u8_t fg_c = (u8_t)(fg);
  u8_t bg_c = (u8_t)(bg);
  u8_t cc = fg_c | (u8_t)(bg_c << 4);
  return (ch_t)(cc);
}

void screen_init()
{
  _video_buffer = (ch_t *)0xB8000;
  _col_ptr = 0;
  _row_ptr = 0;
}

void screen_clear()
{
  // memset((char*)VIDEO_ADDRESS, 0, 2 * SCREEN_HEIGHT * SCREEN_WIDTH);
  for (usz_t i = 0; i < 2 * SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
    _video_buffer[i] = '\0';
  }  

  _col_ptr = 0;
  _row_ptr = 0;
}

base_private void _video_buffer_offset_verify(usz_t offset)
{
  kernel_assert(offset < (2 * SCREEN_WIDTH * SCREEN_HEIGHT));
}

void screen_write_at(
  ch_t c, 
  screen_color_t fg, 
  screen_color_t bg,
  usz_t row, 
  usz_t col
)
{
  usz_t offset = 2 * (row * SCREEN_WIDTH + col);
  ch_t cc = _color_code(fg, bg);

  _video_buffer_offset_verify(offset + 1);
  _video_buffer[offset] = c;
  _video_buffer[offset + 1] = cc;
}

void screen_write_str(
  const char *str,
  screen_color_t fg,
  screen_color_t bg,
  usz_t row,
  usz_t col)
{
  for (usz_t i = 0; (i + col) < SCREEN_WIDTH; i++) {
    if (str[i] == '\0') {
      break;
    }
    screen_write_at(str[i], fg, bg, row, i + col);
  }
}

void screen_write_uint(
  u64_t val, 
  screen_color_t fg, 
  screen_color_t bg, 
  usz_t row,
  usz_t col) 
{
  base_private const usz_t _MSG_LEN = 128;
  ch_t msg[_MSG_LEN];
  usz_t msg_ptr;

  msg_ptr = str_buf_marshal_uint(msg, 0, _MSG_LEN, val);
  msg[msg_ptr] = '\0';

  screen_write_str(msg, fg, bg, row, col); 
}
