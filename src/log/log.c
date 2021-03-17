#include "log.h"
#include "containers_string.h"
#include "drivers_serial.h"
#include "kernel_panic.h"
#include "video.h"

base_private const ch_t *_LINE_PREFIX_LEVEL[LOG_LEVEL_FATAL + 1] = { "[BTE]",
  "[DBG]", "[INF]", "[WRN]", "[ERR]", "[FAT]" };
base_private const usz_t _LINE_PREFIX_LEVEL_LEN = 6;

base_private usz_t _screen_row = 0;
base_private usz_t _screen_col = 0;

base_private bo_t _write_screen = false;

void log_enable_video_write(void)
{
  _write_screen = true;
}

base_private void log_str_len(log_level_t lv, const ch_t *str, usz_t len)
{
  serial_write_str(str, len);

  if (_write_screen) {
    usz_t write_len;
    usz_t video_width = video_char_col_max();

    kernel_assert_d(_screen_col <= video_width);
    if ((video_width - _screen_col) < len) {
      write_len = video_width - _screen_col;
    } else {
      write_len = len;
    }
    for (usz_t i = 0; i < write_len; i++) {
      video_draw_char(_screen_row, _screen_col++, (u8_t)str[i]);
    }
  }

  base_mark_unuse(lv);
}

base_private void log_str(log_level_t lv, const ch_t *str)
{
  log_str_len(lv, str, str_len(str));
}

base_private void log_uint(log_level_t lv, u64_t uval)
{
  const usz_t _MSG_CAP = 64;
  ch_t msg[_MSG_CAP];
  usz_t msg_len = str_buf_marshal_uint(msg, 0, _MSG_CAP, uval);
  log_str_len(lv, msg, msg_len);
}

base_private const ch_t *_file_strip(const ch_t *file)
{
  const ch_t *res = file;
  usz_t idx = 0;
  uin_t score = 0;

  while (true) {
    if (file[idx] == '0') {
      break;
    }

    switch (score) {
    case 0:
      if (file[idx] == '/') {
        score++;
      }
      break;
    case 1:
      if (file[idx] == 's') {
        score++;
      } else {
        score = 0;
      }
      break;
    case 2:
      if (file[idx] == 'r') {
        score++;
      } else {
        score = 0;
      }
      break;
    case 3:
      if (file[idx] == 'c') {
        score++;
      } else {
        score = 0;
      }
      break;
    case 4:
      if (file[idx] == '/') {
        score++;
      } else {
        score = 0;
      }
      break;
    case 5:
      res = file + idx;
      score++;
      break;
    default:
      kernel_panic("Unreachable code.");
      break;
    }

    kernel_assert_d(score <= 6);

    if (score == 6) {
      break;
    } else {
      idx++;
    }
  }
  return res;
}

base_private void _log_line_start(log_level_t lv, const ch_t *file, usz_t line)
{
  log_str_len(lv, _LINE_PREFIX_LEVEL[lv], _LINE_PREFIX_LEVEL_LEN);
  log_str_len(lv, " ", 1);
  log_str(lv, _file_strip(file));
  log_str_len(lv, ":", 1);
  log_uint(lv, line);
  log_str_len(lv, " ", 1);
}

base_private void _log_line_end(log_level_t lv)
{
  if (_write_screen) {
    _screen_row = (_screen_row + 1) % video_char_row_max();
    _screen_col = 0;
  }
  serial_write_str("\r\n", 2);
  base_mark_unuse(lv);
}

void _log_builtin_test_pass(const ch_t *test_name, const ch_t *file, usz_t line)
{
  _log_line_start(LOG_LEVEL_BUILTIN_TEST, file, line);
  log_str(LOG_LEVEL_BUILTIN_TEST, test_name);
  log_str(LOG_LEVEL_BUILTIN_TEST, " .. Passed.");
  _log_line_end(LOG_LEVEL_BUILTIN_TEST);
}

void _log_line_format_v(
    log_level_t lv, const ch_t *file, usz_t line, const ch_t *format, ...)
{
  base_private const usz_t BUF_CAP = 256;
  ch_t buf[BUF_CAP];
  usz_t buf_len;
  va_list list;

  _log_line_start(lv, file, line);

  va_start(list, format);
  buf_len =
      str_buf_marshal_format_v(buf, 0, BUF_CAP, format, str_len(format), list);
  va_end(list);

  log_str_len(lv, buf, buf_len);

  _log_line_end(lv);
}
