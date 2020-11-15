#include "log.h"
#include "containers_string.h"
#include "kernel_panic.h"
#include "panel.h"
#include "drivers_serial.h"

void log_info_str_len(const ch_t *str, usz_t len)
{
  serial_write_str("[INF] ", 6);
  serial_write_str(str, len);
}

void log_info_str(const ch_t *str)
{
  log_info_str_len(str, str_len(str));
}

void log_info_uint(u64_t uval)
{
  const usz_t _MSG_CAP = 64;
  ch_t msg[_MSG_CAP];
  usz_t msg_len = str_buf_marshal_uint(msg, 0, _MSG_CAP, uval);
  log_info_str_len(msg, msg_len);
}

void log_info_str_line_len(const ch_t *str, usz_t len)
{
  log_info_str_len(str, len);
 serial_write_str("\r\n", 2);
}

void log_info_str_line(const ch_t *str) {
  log_info_str_line_len(str, str_len(str));
}
