#include "log.h"
#include "containers_string.h"
#include "drivers_serial.h"
#include "kernel_panic.h"
#include "panel.h"

#define _SIZE_COUNT 6
base_private const u64_t _SIZE_UNITS[_SIZE_COUNT] = {
  u64_literal(1024) * u64_literal(1024) * u64_literal(1024) *
      u64_literal(1024) * u64_literal(1024) * u64_literal(1024),
  u64_literal(1024) * u64_literal(1024) * u64_literal(1024) *
      u64_literal(1024) * u64_literal(1024),
  u64_literal(1024) * u64_literal(1024) * u64_literal(1024) * u64_literal(1024),
  1024 * 1024 * 1024, 1024 * 1024, 1024
};

base_private const ch_t *_SIZE_NOTES[_SIZE_COUNT] = { "ZB", "PB", "TB", "GB",
  "MB", "KB" };

base_private const ch_t *_LINE_PREFIX_LEVEL[LOG_LEVEL_FATAL + 1] = { "[DBG]",
  "[INF]", "[WRN]", "[ERR]", "[FAT]" };

void log_str_len(log_level_t lv, const ch_t *str, usz_t len)
{
  serial_write_str(str, len);
  base_mark_unuse(lv);
}

void log_str(log_level_t lv, const ch_t *str)
{
  log_str_len(lv, str, str_len(str));
}

void log_uint(log_level_t lv, u64_t uval)
{
  const usz_t _MSG_CAP = 64;
  ch_t msg[_MSG_CAP];
  usz_t msg_len = str_buf_marshal_uint(msg, 0, _MSG_CAP, uval);
  log_str_len(lv, msg, msg_len);
}

void log_uint_of_size(log_level_t lv, u64_t uval)
{
  for (usz_t i = 0; i < _SIZE_COUNT; i++) {
    if ((uval / _SIZE_UNITS[i]) > 0) {
      log_uint(lv, uval / _SIZE_UNITS[i]);
      log_str_len(lv, _SIZE_NOTES[i], 2);
    }
    uval = uval % _SIZE_UNITS[i];
  }

  if ((uval % _SIZE_UNITS[0]) > 0) {
    log_uint(lv, uval % _SIZE_UNITS[0]);
  }
}

void log_line_start(log_level_t lv)
{
  serial_write_str(_LINE_PREFIX_LEVEL[lv], 6);
}

void log_line_end(log_level_t lv)
{
  serial_write_str("\r\n", 2);
  base_mark_unuse(lv);
}
