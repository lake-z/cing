#ifndef ___CONTAINERS_STRING
#define ___CONTAINERS_STRING

#include "base.h"
#include "mm.h"
#include <stdarg.h>

bo_t byte_bit_get(byte_t byte, usz_t bit);
byte_t byte_bit_set(byte_t byte, usz_t bit);
byte_t byte_bit_clear(byte_t byte, usz_t bit);
byte_t byte_get(u64_t ival, usz_t byte);

usz_t str_len(const ch_t *str);

/*
 * Add unsigned int value into result string.
 * @return Bytes added into result string
 */
usz_t str_buf_marshal_uint(ch_t *buf, /* Result string */
    const usz_t buf_off,              /* Offset to write value */
    const usz_t buf_len, /* Total length, no overwrite is gurateed */
    const u64_t val      /* Value to be added */
);

usz_t str_buf_marshal_uint_in_size(
    ch_t *buf, const usz_t buf_off, const usz_t buf_len, const u64_t val);
ch_t *str_buf_marshal_uint_in_size_new(mm_allocator_t *all, const u64_t val);

/* Add a string to specified postion of result string.
 * @return Bytes added to result string
 */
usz_t str_buf_marshal_str(ch_t *buf, /* Result string */
    const usz_t buf_off,             /* Offset to write value */
    const usz_t buf_len, /* Total length, no overwrite is gurateed */
    const ch_t *str,     /* String to be added */
    usz_t str_len        /* String length */
);

usz_t str_buf_marshal_bytes_in_hex(ch_t *buf,
    const usz_t buf_off,
    const usz_t buf_len,
    const byte_t *str,
    usz_t str_len);

usz_t str_buf_marshal_terminator(
    ch_t *buf, const usz_t buf_off, const usz_t buf_len);

usz_t str_buf_marshal_format_v(ch_t *buf,
    const usz_t buf_off,
    const usz_t buf_len,
    const ch_t *format,
    const usz_t format_len,
    va_list va);

usz_t str_buf_marshal_format(
    ch_t *buf, usz_t buf_off, usz_t buf_len, const char *format, ...)
    base_check_format(4, 5);

#endif
