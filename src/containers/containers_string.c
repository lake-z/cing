#include "containers_string.h"
#include "drivers_screen.h"
#include "kernel_panic.h"

usz_t str_len(const ch_t *str)
{
  usz_t len;

  kernel_assert(str != NULL);

  len = 0;
  while (str[len] != '\0') {
    len++;
  }
  return len;
}

usz_t str_buf_marshal_str(ch_t *buf,
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

base_private void _buf_marshal_byte_in_hex(
    ch_t *buf, const usz_t buf_off, const usz_t buf_len, byte_t byte)
{
  u64_t val;

  kernel_assert(buf_off < (buf_len - 2));

  val = (byte >> 4) & 0x0f;
  if (val < 10) {
    buf[buf_off] = (ch_t)(val + '0');
  } else {
    kernel_assert(val < 16);
    buf[buf_off] = (ch_t)(val - 10 + 'A');
  }

  val = byte & 0x0f;
  if (val < 10) {
    buf[buf_off + 1] = (ch_t)(val + '0');
  } else {
    kernel_assert(val < 16);
    buf[buf_off + 1] = (ch_t)(val - 10 + 'A');
  }
}

usz_t str_buf_marshal_bytes_in_hex(ch_t *buf,
    const usz_t buf_off,
    const usz_t buf_len,
    const byte_t *str,
    usz_t str_len)
{
  usz_t buf_ptr;

  kernel_assert(buf != NULL);
  kernel_assert(str != NULL);
  kernel_assert(str_len > 0);
  kernel_assert(buf_len > 0);
  kernel_assert(buf_off < buf_len);
  kernel_assert((buf_len - buf_off) > (str_len * 3 - 1));

  buf_ptr = buf_off;

  _buf_marshal_byte_in_hex(buf, buf_ptr, buf_len, str[0]);
  buf_ptr += 2;
  for (usz_t i = 1; i < str_len; i++) {
    buf[buf_ptr++] = ' ';
    _buf_marshal_byte_in_hex(buf, buf_ptr, buf_len, str[i]);
    buf_ptr += 2;
  }

  kernel_assert(buf_ptr < buf_len);
  return buf_ptr - buf_off;
}

usz_t str_buf_marshal_uint(
    ch_t *buf, const usz_t buf_off, const usz_t buf_len, const u64_t val)
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
  } while (val_pro > 0);
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

  kernel_assert(buf_off_pro == buf_off);
  return digit_cnt;
}

usz_t str_buf_marshal_terminator(
    ch_t *buf, const usz_t buf_off, const usz_t buf_len)
{
  kernel_assert((buf_len - buf_off) > 1);
  buf[buf_off] = '\0';
  return 1;
}

bo_t byte_bit_get(byte_t byte, usz_t bit)
{
  kernel_assert(bit < 8);
  return byte & (1 << bit);
}

byte_t byte_bit_set(byte_t byte, usz_t bit)
{
  kernel_assert(bit < 8);
  return (byte |= ((byte_t)(1 << bit)));
}

byte_t byte_bit_clear(byte_t byte, usz_t bit)
{
  kernel_assert(bit < 8);
  return (byte &= ((byte_t) ~(((byte_t)(1 << bit)))));
}
