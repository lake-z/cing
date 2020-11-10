#include "log.h"
#include "containers_string.h"
#include "kernel_panic.h"
#include "panel.h"
#include "drivers_serial.h"

/*
void log_info(const ch_t *str, usz_t len)
{
  base_private const usz_t _MSG_LEN = 80;
  ch_t msg[_MSG_LEN];
  usz_t msg_len;
  const ch_t *msg_part;
  panel_text_t *txt = panel_get_log_text();
  usz_t width = panel_text_width(txt);
  bo_t first_row = true;
  bo_t ok;

  while (len > 0) {
    usz_t write_len;

    if (first_row) {
      msg_part = "INF ";
      first_row = false;
    } else {
      msg_part = "    ";
    }
    msg_len = 0;
    msg_len += str_buf_marshal_str(
        msg, msg_len, _MSG_LEN, msg_part, str_len(msg_part));

    if ((width - msg_len) > len) {
      write_len = len;
    } else {
      write_len = width - msg_len;
    }
    msg_len += str_buf_marshal_str(msg, msg_len, _MSG_LEN, str, write_len);

    ok = panel_text_write_row(txt, msg, msg_len);
    kernel_assert(ok == true);

    str += write_len;
    len -= write_len;
  }

  panel_text_draw(txt);
}
*/

void log_info_len(const ch_t *str, usz_t len)
{
  serial_write_str("[INF] ", 6);
  serial_write_str(str, len);
  serial_write_str("\r\n", 2);
}

void log_info(const ch_t *str) {
  log_info_len(str, str_len(str));
}
