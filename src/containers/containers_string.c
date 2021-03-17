#include "containers_string.h"
#include "drivers_screen.h"
#include "kernel_panic.h"

#define _SIZE_COUNT 6
base_private const u64_t _SIZE_UNITS[_SIZE_COUNT] = {
  u64_literal(1024) * u64_literal(1024) * u64_literal(1024) *
      u64_literal(1024) * u64_literal(1024) * u64_literal(1024),
  u64_literal(1024) * u64_literal(1024) * u64_literal(1024) *
      u64_literal(1024) * u64_literal(1024),
  u64_literal(1024) * u64_literal(1024) * u64_literal(1024) * u64_literal(1024),
  1024 * 1024 * 1024,
  1024 * 1024,
  1024,
};

base_private const ch_t *_SIZE_NOTES[_SIZE_COUNT] = { "ZB", "PB", "TB", "GB",
  "MB", "KB" };

usz_t str_len(const ch_t *str)
{
  usz_t len = 0;
  while (str[len] != '\0') {
    len++;
  }
  return len;
}

base_private bool str_char_is_digit(ch_t ch)
{
  return ch >= '0' && ch <= '9';
}

base_private u64_t str_char_to_uint(ch_t ch)
{
  return (u64_t)(ch - '0');
}

base_private u64_t str_to_uint(
    const char *str, const usz_t str_off, const usz_t str_len, usz_t *out_len)

{
  u64_t val = 0;
  usz_t str_ptr = str_off;
  for (; str_ptr < str_len; str_ptr++) {
    if (str_char_is_digit(str[str_ptr])) {
      val = val * 10 + str_char_to_uint(str[str_ptr]);
    } else {
      break;
    }
  }
  kernel_assert(str_ptr > str_off);
  *out_len = str_ptr - str_off;
  return val;
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

usz_t str_buf_marshal_uint_in_size(
    ch_t *buf, const usz_t buf_off, const usz_t buf_len, const u64_t val)
{
  usz_t buf_ptr = buf_off;

  kernel_assert(buf_off < buf_len);

  for (usz_t i = 0; i < _SIZE_COUNT; i++) {
    u64_t part = val / _SIZE_UNITS[i];
    if (part > 0) {
      buf_ptr += str_buf_marshal_uint(buf, buf_ptr, buf_len, part);
      kernel_assert_d((buf_ptr + 2) <= buf_len);
      str_buf_marshal_str(buf, buf_ptr, buf_len, _SIZE_NOTES[i], 2);
    }
  }
  kernel_assert_d(buf_ptr <= buf_len);
  return buf_ptr - buf_off;
}

ch_t *str_buf_marshal_uint_in_size_new(mm_allocator_t *all, const u64_t val)
{
  usz_t len;
  usz_t buf_len = 128;
  ch_t *ret = (ch_t *)mm_allocate(all, buf_len, 1);
  len = str_buf_marshal_uint_in_size(ret, 0, buf_len, val);
  kernel_assert(len < buf_len);
  return ret;
}

usz_t str_buf_marshal_terminator(
    ch_t *buf, const usz_t buf_off, const usz_t buf_len)
{
  kernel_assert((buf_len - buf_off) > 1);
  buf[buf_off] = '\0';
  return 1;
}

usz_t str_buf_marshal_format_v(ch_t *buf,
    const usz_t buf_off,
    const usz_t buf_len,
    const ch_t *format,
    const usz_t format_len,
    va_list va)
{
  usz_t buf_ptr;
  kernel_assert(buf_off < buf_len);

  buf_ptr = buf_off;
  for (usz_t format_ptr = 0; format_ptr < format_len; format_ptr++) {
    if (base_likely(format[format_ptr] != '%')) {
      buf[buf_ptr++] = format[format_ptr];
    } else {
      format_ptr++;
      if (base_unlikely(format[format_ptr] == '%')) {
        buf[buf_ptr++] = '%';
      } else if (format[format_ptr] == 's') {
        const ch_t *str = va_arg(va, const char *);
        buf_ptr +=
            str_buf_marshal_str(buf, buf_ptr, buf_len, str, str_len(str));
      } else if (format[format_ptr] == 'l') {
        format_ptr++;
        if (format[format_ptr] == 'u') {
          u64_t uval = va_arg(va, u64_t);
          buf_ptr += str_buf_marshal_uint(buf, buf_ptr, buf_len, uval);
        } else {
          kernel_panic("TODO");
        }
      } else if (format[format_ptr] == '.') {
        const ch_t *str;
        usz_t strlen;
        usz_t strlenlen;
        format_ptr++;
        strlen = str_to_uint(format, format_ptr, format_len, &strlenlen);
        format_ptr += strlenlen;
        str = va_arg(va, const char *);
        for (usz_t str_ptr = 0; str_ptr < strlen; str_ptr++) {
          if (str[str_ptr] == '\0') {
            break;
          } else {
            kernel_assert(buf_ptr < buf_len);
            buf[buf_ptr++] = str[str_ptr];
          }
        }
      } else {
        kernel_panic("TODO");
      }
    }
  }

  kernel_assert(buf_ptr <= buf_len);

  return buf_ptr - buf_off;
}

base_check_format(4, 5) usz_t str_buf_marshal_format(
    ch_t *buf, usz_t buf_off, usz_t buf_len, const char *format, ...)
{
  usz_t len;
  va_list list;
  va_start(list, format);
  len = str_buf_marshal_format_v(
      buf, buf_off, buf_len, format, str_len(format), list);
  kernel_assert((buf_off + len) <= buf_len);
  va_end(list);
  return len;
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

byte_t byte_get(u64_t ival, usz_t byte)
{
  kernel_assert(byte < 4);
  u64_t v64 = (ival >> (u64_t)(8 * byte)) & u64_literal(0xff);
  kernel_assert(v64 <= U8_MAX);
  return (byte_t)v64;
}
