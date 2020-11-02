#include "panel.h"
#include "drivers_screen.h"
#include "kernel_panic.h"

base_private const screen_color_t _NAV_BG = SCREEN_COLOR_LIGHT_GREY;

base_private void panel_write_char(
    ch_t c, screen_color_t fg, screen_color_t bg, usz_t row, usz_t col)
{
  kernel_assert(row < SCREEN_HEIGHT);
  kernel_assert(col < SCREEN_WIDTH);

  screen_write_at((byte_t)c, fg, bg, row, col);
}

/* Must be able to write in one row */
base_private void panel_write_str(
    const ch_t *str, screen_color_t fg, screen_color_t bg, usz_t row, usz_t col)
{
  kernel_assert(str != NULL);

  for (usz_t i = 0; (i + col) < SCREEN_WIDTH; i++) {
    if (str[i] == '\0') {
      break;
    } else {
      panel_write_char(str[i], fg, bg, row, col + i);
    }
  }
}

base_private void panel_nav_init(void)
{
  const char *item = "Log";

  for (usz_t c = 0; c < SCREEN_WIDTH; c++) {
    screen_fill_bg_at(_NAV_BG, 0, c);
  }
  panel_write_char(item[0], SCREEN_COLOR_RED, _NAV_BG, 0, 2);
  panel_write_str(item + 1, SCREEN_COLOR_BLACK, _NAV_BG, 0, 3);
}

void panel_start(void)
{
  screen_clear(SCREEN_COLOR_BLACK);
  panel_nav_init();
}
