#include "containers_string.h"
#include "kernel_panic.h"
#include "panel_private.h"

base_private const screen_color_t _NAV_BG = SCREEN_COLOR_LIGHT_GREY;
base_private panel_text_t _log_txt;

base_private void panel_write_char(ch_t c, usz_t row, usz_t col)
{
  kernel_assert(row < SCREEN_HEIGHT);
  kernel_assert(col < SCREEN_WIDTH);

  screen_write_at((byte_t)c, row, col);
}

/* Must be able to write in one row */
base_private void panel_write_str(const ch_t *str, usz_t row, usz_t col)
{
  kernel_assert(str != NULL);

  for (usz_t i = 0; (i + col) < SCREEN_WIDTH; i++) {
    if (str[i] == '\0') {
      break;
    } else {
      panel_write_char(str[i], row, col + i);
    }
  }
}

/* Must be able to write in one row */
void panel_write_str_len(const ch_t *str, usz_t str_len, usz_t row, usz_t col)
{
  kernel_assert((col + str_len) < SCREEN_WIDTH);

  for (usz_t i = 0; i < str_len; i++) {
    panel_write_char(str[i], row, col + i);
  }
}

base_private void panel_nav_init(void)
{
  const char *item = "Log";

  for (usz_t c = 0; c < SCREEN_WIDTH; c++) {
    screen_clean_at(_NAV_BG, 0, c);
  }
  panel_write_char(item[0], 0, 2);
  panel_write_str(item + 1, 0, 3);
}

void panel_start(void)
{
  screen_clean(SCREEN_COLOR_BLACK);
  panel_nav_init();

  panel_text_init(&_log_txt, 1, 0, SCREEN_HEIGHT - 2, SCREEN_WIDTH - 1);
  panel_text_draw(&_log_txt);

  bo_t ok;
  ok = panel_text_write_row(&_log_txt, "abc", str_len("abc"));
  kernel_assert(ok);
  panel_text_draw(&_log_txt);
}

panel_text_t *panel_get_log_text()
{
  return &_log_txt;
}
