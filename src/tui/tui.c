/* Text based user interface.
 * Scrann is divided into 8x8 tiles.
 * Currently supports 8x16 VGA chars only, which is 1 tile wide, and 2 tiles
 * tall each. */

#include "containers_string.h"
#include "drivers_vesa.h"
#include "kernel_panic.h"
#include "tui_private.h"

base_private const usz_t _TILE_WIDTH = 8;
base_private const usz_t _TILE_HEIGHT = 8;
base_private const usz_t _TILE_X_MAX = 240;
base_private const usz_t _TILE_Y_MAX = 135;
base_private const usz_t _CHAR_WIDTH = 8;
base_private const usz_t _CHAR_HEIGHT = 16;

typedef struct tui_color {
  u8_t red;
  u8_t green;
  u8_t blue;
} tui_color_t;

typedef struct tui_widget_menu {
  usz_t start_x;
  usz_t start_y;
  usz_t width;
  tui_color_t bg;
} tui_widget_menu_t;

base_private void tui_screen_draw_tile_line(
    usz_t x, usz_t y, usz_t line_no, byte_t line, tui_color_t color)
{
  kernel_assert(line_no < _TILE_HEIGHT);

  usz_t pixel_y = y * _TILE_HEIGHT + line_no;
  for (usz_t i = 0; i < _TILE_WIDTH; i++) {
    if (byte_bit_get(line, 8 - i - 1)) {
      d_vesa_draw_pixel(
          x * _TILE_WIDTH + i, pixel_y, color.red, color.green, color.blue);
    }
  }
}

base_private void tui_screen_draw_tile_full(usz_t x, usz_t y, tui_color_t color)
{
  for (usz_t i = 0; i < _TILE_HEIGHT; i++) {
    tui_screen_draw_tile_line(x, y, i, 0xFF, color);
  }
}

/*
base_private usz_t char_start_tile_x(usz_t row, usz_t col)
{
  base_mark_unuse(row);
  return col * _CHAR_WIDTH / _TILE_WIDTH;
}

base_private usz_t char_start_tile_y(usz_t row, usz_t col)
{
  base_mark_unuse(col);
  return row * _CHAR_HEIGHT / _TILE_HEIGHT;
}
*/

//base_private void tui_screen_draw_char(usz_t row, usz_t col, byte_t ch, tui_color_t fg,
//    tui_color_t bg)
//{
//  usz_t font_start;
//  usz_t start_x;
//  usz_t start_y;
//
//  kernel_assert(row < tui_screen_char_row_max());
//  kernel_assert(col < tui_screen_char_col_max());
//
//  font_start = (usz_t)(ch * _CHAR_HEIGHT); /* 16 tile lines per char */
//  start_x = char_start_tile_x(row, col);
//  start_y = char_start_tile_y(row_col);
//
//  for (usz_t ln = 0; ln < _CHAR_HEIGHT; ln++) {
//    u8_t tile_line = FONT_VGA_8x16[font_start + ln];
//    tui_screen_draw_tile_line(start_x);
//    for (usz_t bit = 0; bit < 8; bit++) {
//      if (byte_bit_get(ln_byte, 8 - bit - 1)) {
//        d_vesa_draw_pixel(col * 8 + bit, row * 16 + ln, 255, 255, 255);
//      }
//    }
//  }
//}

base_private void tui_widget_menu_draw(tui_widget_menu_t *menu)
{
  for (usz_t i = 0; i < menu->width; i++) {
    tui_screen_draw_tile_full(menu->start_x + i, menu->start_y, menu->bg);
    tui_screen_draw_tile_full(menu->start_x + i, menu->start_y + 1, menu->bg);
  }
}

void tui_start(void)
{
  tui_widget_menu_t top;

  kernel_assert(d_vesa_get_width() / _TILE_WIDTH >= _TILE_X_MAX);
  kernel_assert(d_vesa_get_height() / _TILE_HEIGHT >= _TILE_Y_MAX);

  top.start_x = 0;
  top.start_y = 0;
  top.width = _TILE_X_MAX;
  top.bg.red = 187;
  top.bg.green = 187;
  top.bg.blue = 187;

  tui_widget_menu_draw(&top);
}
