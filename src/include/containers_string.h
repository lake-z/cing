#ifndef ___CONTAINERS_STRING
#define ___CONTAINERS_STRING

#include "base.h"

bo_t byte_bit_get(byte_t byte, usz_t bit);
byte_t byte_bit_set(byte_t byte, usz_t bit);
byte_t byte_bit_clear(byte_t byte, usz_t bit);

usz_t str_len(const char *str);

/*
 * Add unsigned int value into result string.
 * @return Bytes added into result string
 */
usz_t str_buf_marshal_uint(
  ch_t *buf, /* Result string */
  const usz_t buf_off, /* Offset to write value */
  const usz_t buf_len, /* Total length, no overwrite is gurateed */
  const u64_t val /* Value to be added */
);

/* Add a string to specified postion of result string. 
 * @return Bytes added to result string
 */
usz_t str_buf_marshal_str(
  ch_t *buf, /* Result string */
  const usz_t buf_off, /* Offset to write value */
  const usz_t buf_len, /* Total length, no overwrite is gurateed */
  const ch_t *str, /* String to be added */
  usz_t str_len /* String length */
);

usz_t str_buf_marshal_terminator(
  ch_t *buf,
  const usz_t buf_off,
  const usz_t buf_len
);
#endif
