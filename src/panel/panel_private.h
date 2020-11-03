#ifndef ___PANEL_PRIVATE
#define ___PANEL_PRIVATE

#include "drivers_screen.h"
#include "panel.h"

struct panel_text {
  usz_t start_row;
  usz_t start_col;
  usz_t end_row;
  usz_t end_col;
  screen_color_t box_color;
  screen_color_t body_color;
  /* Optimize those very big staticlly buffer after dynamic memory management
   * is implemented. */
  ch_t buf[1024 * 1024];
  usz_t row_lens[1024 * 1024];
  usz_t buf_len;

  usz_t buf_tail;
  usz_t window_head;
  usz_t write_cnt;
};

void panel_write_str_len(const ch_t *str, usz_t str_len, usz_t row, usz_t col);

void panel_text_init(
    panel_text_t *tx, usz_t row0, usz_t col0, usz_t rowx, usz_t colx);
#endif
