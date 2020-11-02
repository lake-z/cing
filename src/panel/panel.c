#include "panel.h"
#include "containers_string.h"
#include "drivers_screen.h"
#include "kernel_panic.h"

typedef struct {
  usz_t start_row;
  usz_t start_col;
  usz_t end_row;
  usz_t end_col;
  screen_color_t box_color;
  screen_color_t body_color;

  ch_t buf[1024 * 1024];
  usz_t buf_len;
  usz_t buf_tail;

  usz_t write_row_cnt;
  usz_t window_start_row;
} panel_text_t;

base_private const screen_color_t _NAV_BG = SCREEN_COLOR_LIGHT_GREY;

base_private void panel_text_init(
    panel_text_t *tx, usz_t row0, usz_t col0, usz_t rowx, usz_t colx)
{
  kernel_assert(tx != NULL);
  tx->start_row = row0;
  tx->start_col = col0;
  tx->end_row = rowx;
  tx->end_col = colx;
  tx->box_color = SCREEN_COLOR_BLUE;

  /* TODO: Buffer length divided by text area width, trailing space will be 
   * wasted, allocate memory dynamiclly to match buf length and area width 
   * after mm is implemented. */
  tx->buf_len = 1024 * 1024 / (tx->end_col - tx->start_col - 1);
  tx->buf_tail = 0;
  tx->write_row_cnt = 0;
  tx->window_start_row = 0;
}

base_private void panel_text_draw(panel_text_t *txt)
{
  kernel_assert(txt != NULL);
  screen_box_thin_corner_nw_at(txt->start_row, txt->start_col);
  screen_set_fg_at(txt->box_color, txt->start_row, txt->start_col);

  screen_box_thin_corner_ne_at(txt->start_row, txt->end_col);
  screen_set_fg_at(txt->box_color, txt->start_row, txt->end_col);

  screen_box_thin_corner_sw_at(txt->end_row, txt->start_col);
  screen_set_fg_at(txt->box_color, txt->end_row, txt->start_col);

  screen_box_thin_corner_se_at(txt->end_row, txt->end_col);
  screen_set_fg_at(txt->box_color, txt->end_row, txt->end_col);

  for (usz_t i = txt->start_row + 1; i < txt->end_row; i++) {
    screen_box_thin_border_v_at(i, txt->start_col);
    screen_set_fg_at(txt->box_color, i, txt->start_col);

    screen_box_thin_border_v_at(i, txt->end_col);
    screen_set_fg_at(txt->box_color, i, txt->end_col);
  }

  for (usz_t i = txt->start_col + 1; i < txt->end_col; i++) {
    screen_box_thin_border_h_at(txt->start_row, i);
    screen_set_fg_at(txt->box_color, txt->start_row, i);

    screen_box_thin_border_h_at(txt->end_row, i);
    screen_set_fg_at(txt->box_color, txt->end_row, i);
  }
}

base_private usz_t panel_text_width(panel_text_t *txt)
{
  return txt->end_col - txt->start_col - 1;
}

base_private usz_t panel_text_row_max(panel_text_t *txt)
{
  return (txt->buf_len) / panel_text_width(txt);
}

base_private usz_t panel_text_get_row(panel_text_t *txt, usz_t row)
{
  kernel_assert(txt != NULL);
  kernel_assert(row < panel_text_row_max(txt));
  return panel_text_width(txt) * row;
}

base_private void panel_text_write_row(
    panel_text_t *txt, const ch_t *str, usz_t str_len)
{
  usz_t off;
  kernel_assert(txt != NULL);
  kernel_assert(str != NULL);
  kernel_assert(str_len < panel_text_width(txt));

  off = panel_text_get_row(txt, txt->buf_tail);
  for (usz_t i = 0; i < str_len; i++) {
    txt->buf[off + i] = str[i];
  }

  (txt->buf_tail) = ((txt->buf_tail) + 1) % panel_text_row_max(txt);
  if (txt->write_row_cnt < panel_text_row_max(txt)) {
    txt->write_row_cnt++;
  }
}

base_private void panel_write_char(
    ch_t c, screen_color_t fg, usz_t row, usz_t col)
{
  kernel_assert(row < SCREEN_HEIGHT);
  kernel_assert(col < SCREEN_WIDTH);

  screen_write_at((byte_t)c, row, col);
  screen_set_fg_at(fg, row, col);
}

/* Must be able to write in one row */
base_private void panel_write_str(
    const ch_t *str, screen_color_t fg, usz_t row, usz_t col)
{
  kernel_assert(str != NULL);

  for (usz_t i = 0; (i + col) < SCREEN_WIDTH; i++) {
    if (str[i] == '\0') {
      break;
    } else {
      panel_write_char(str[i], fg, row, col + i);
    }
  }
}

base_private void panel_nav_init(void)
{
  const char *item = "Log";

  for (usz_t c = 0; c < SCREEN_WIDTH; c++) {
    screen_clean_at(_NAV_BG, 0, c);
  }
  panel_write_char(item[0], SCREEN_COLOR_RED, 0, 2);
  panel_write_str(item + 1, SCREEN_COLOR_BLACK, 0, 3);
}

void panel_start(void)
{
  panel_text_t txt;

  screen_clean(SCREEN_COLOR_BLACK);
  panel_nav_init();
  panel_text_init(&txt, 1, 0, SCREEN_HEIGHT - 2, SCREEN_WIDTH - 1);
  panel_text_draw(&txt);

  panel_text_write_row(&txt, "abc", str_len("abc"));
}
