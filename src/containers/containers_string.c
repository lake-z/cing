#include "containers_string.h"
#include "drivers_screen.h"
#include "kernel_panic.h"

usz_t str_len(const ch_t *str)
{
  usz_t len;

  kernel_assert(str != NULL);

  len = 0;
  while(str[len] != '\0') {
    len++;
  }
  return len;
}

usz_t str_buf_marshal_str(
  ch_t *buf,
  const usz_t buf_off,
  const usz_t buf_len,
  const ch_t *str,
  usz_t str_len)
{
  kernel_assert((buf_len - buf_off) > str_len);
  for (usz_t i = 0; i < str_len; i++) {
    buf[buf_off + i] = str[i];
  }
  return str_len;
}

usz_t str_buf_marshal_uint(
  ch_t *buf,
  const usz_t buf_off,
  const usz_t buf_len,
  const u64_t val)
{
  u64_t val_pro;
  usz_t buf_off_pro;
  usz_t digit_cnt;

  /* Counting digits in total*/
  digit_cnt = 0;
  val_pro = val;
  buf_off_pro = buf_off;
  do {
    val_pro /= 10;
    digit_cnt++;
  } while(val_pro > 0);
  kernel_assert((buf_len - buf_off) > digit_cnt);

  val_pro = val;
  buf_off_pro = buf_off + digit_cnt - 1;
  while (true) {
    usz_t digit = val_pro % 10;
    buf[buf_off_pro] = (ch_t)(digit + '0');

    val_pro /= 10;
    if (val_pro == 0) {
      break;
    }
    buf_off_pro--;
  }
  screen_write_str("val", SCREEN_COLOR_RED, SCREEN_COLOR_BLACK, 
    val, SCREEN_WIDTH - 20);
   screen_write_str("digit_cnt", SCREEN_COLOR_RED, SCREEN_COLOR_BLACK, 
    digit_cnt, SCREEN_WIDTH - 20);
 
  kernel_assert(buf_off_pro == buf_off);
  return digit_cnt;
}

bo_t byte_is_bit_set(byte_t byte, usz_t bit)
{
  kernel_assert(bit < 8);
  return byte & (1 << (bit - 1));
}
