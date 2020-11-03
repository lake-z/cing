#include "kernel_panic.h"
#include "panel_private.h"

void panel_text_init(
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
  tx->write_cnt = 0;
  tx->window_head = 0;
}

usz_t panel_text_width(panel_text_t *txt)
{
  kernel_assert(txt != NULL);
  return txt->end_col - txt->start_col - 1;
}

base_private usz_t panel_text_height(panel_text_t *txt)
{
  kernel_assert(txt != NULL);
  return txt->end_row - txt->start_row - 1;
}

base_private usz_t _buf_row_total(panel_text_t *txt)
{
  return (txt->buf_len) / panel_text_width(txt);
}

base_private usz_t _buf_row_offs(panel_text_t *txt, usz_t row)
{
  kernel_assert(txt != NULL);
  kernel_assert(row < _buf_row_total(txt));
  return panel_text_width(txt) * row;
}

base_private usz_t _buf_tail_next(panel_text_t *t)
{
  kernel_assert(t != NULL);
  return (t->buf_tail + 1) % _buf_row_total(t);
}

base_private bo_t panel_text_is_full(panel_text_t *t)
{
  kernel_assert(t != NULL);
  return _buf_tail_next(t) == t->window_head;
}

base_private usz_t _txt_start_row(panel_text_t *txt)
{
  return txt->start_row + 1;
}

base_private usz_t _txt_start_col(panel_text_t *txt)
{
  return txt->start_col + 1;
}

/*
base_private usz_t _txt_end_row(panel_text_t *txt)
{
  return txt->end_row - 1;
}

base_private usz_t _txt_end_col(panel_text_t *txt)
{
  return txt->end_col - 1;
}
*/

base_private void _draw_txt(panel_text_t *txt)
{
  usz_t row_cnt = panel_text_height(txt);
  if (txt->write_cnt < row_cnt) {
    row_cnt = txt->write_cnt;
  }

  for (usz_t i = 0; i < row_cnt; i++) {
    usz_t off = _buf_row_offs(txt, i + txt->window_head);
    panel_write_str_len(txt->buf + off, txt->row_lens[i + txt->window_head],
        _txt_start_row(txt) + i, _txt_start_col(txt));
  }
}

void panel_text_draw(panel_text_t *txt)
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

  _draw_txt(txt);
}

/*
 * Write one row to the end of text. 
 * str_len must be less than textarea width
 * return true for succeed, false for failure because not enough buffer space
 */
bo_t panel_text_write_row(panel_text_t *txt, const ch_t *str, usz_t str_len)
{
  usz_t off;
  kernel_assert(txt != NULL);
  kernel_assert(str != NULL);
  kernel_assert(str_len < panel_text_width(txt));

  if (panel_text_is_full(txt)) {
    return false;
  }

  off = _buf_row_offs(txt, txt->buf_tail);
  for (usz_t i = 0; i < str_len; i++) {
    txt->buf[off + i] = str[i];
  }
  txt->row_lens[txt->buf_tail] = str_len;

  (txt->buf_tail) = _buf_tail_next(txt);
  txt->write_cnt++;
  return true;
}
