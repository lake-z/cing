#ifndef ___DRIVERS_SCREEN
#define ___DRIVERS_SCREEN

#include "base.h"

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25

typedef enum {
  SCREEN_COLOR_BLACK = 0,
  SCREEN_COLOR_BLUE = 1,
  SCREEN_COLOR_GREEN = 2,
  SCREEN_COLOR_CYAN = 3,
  SCREEN_COLOR_RED = 4,
  SCREEN_COLOR_MAGENTA = 5,
  SCREEN_COLOR_BROWN = 6,
  SCREEN_COLOR_LIGHT_GREY = 7,
  SCREEN_COLOR_DARK_GREY = 8,
  SCREEN_COLOR_LIGHT_BLUE = 9,
  SCREEN_COLOR_LIGHT_GREEN = 10,
  SCREEN_COLOR_LIGHT_CYAN = 11,
  SCREEN_COLOR_LIGHT_RED = 12,
  SCREEN_COLOR_LIGHT_MAGENTA = 13,
  SCREEN_COLOR_LIGHT_BROWN = 14,
  SCREEN_COLOR_WHITE = 15
} screen_color_t;

void screen_init(void);
void screen_cursor_disable(void);
void screen_blink_disable(void);
void screen_write_at(byte_t c, usz_t row, usz_t col);
void screen_set_attr_at(
    screen_color_t fg, screen_color_t bg, usz_t row, usz_t col);
void screen_set_fg_at(screen_color_t fg, usz_t row, usz_t col);
void screen_set_bg_at(screen_color_t bg, usz_t row, usz_t col);
void screen_write_at(byte_t c, usz_t row, usz_t col);
void screen_clean_at(screen_color_t bg, usz_t row, usz_t col);
void screen_box_thin_border_v_at(usz_t row, usz_t col);
void screen_box_thin_border_h_at(usz_t row, usz_t col);
void screen_box_thin_corner_ne_at(usz_t row, usz_t col);
void screen_box_thin_corner_sw_at(usz_t row, usz_t col);
void screen_box_thin_corner_se_at(usz_t row, usz_t col);
void screen_box_thin_corner_nw_at(usz_t row, usz_t col);
void screen_clean(screen_color_t bg);

void screen_write_str(const char *str,
    screen_color_t fg,
    screen_color_t bg,
    usz_t row,
    usz_t col) base_no_null;

void screen_write_uint(u64_t val,
    screen_color_t fg,
    screen_color_t bg,
    usz_t row,
    usz_t col) base_no_null;

void screen_write_byte_hex(byte_t *bytes,
    usz_t len,
    screen_color_t fg,
    screen_color_t bg,
    usz_t row,
    usz_t col) base_no_null;
#endif
