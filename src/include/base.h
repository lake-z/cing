#ifndef ___BASE
#define ___BASE

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef bool bo_t;
typedef unsigned char uch_t;
typedef char ch_t;
typedef signed char i8_t;
typedef signed short i16_t;
typedef int32_t i32_t;
typedef int64_t i64_t;
typedef unsigned char u8_t;
typedef unsigned short u16_t;
typedef uint32_t u32_t;
typedef uint64_t u64_t;
typedef u8_t byte_t;
typedef size_t usz_t;
typedef ssize_t ssz_t;
typedef float f32_t;
typedef double f64_t;

#define i64_literl(x) (x##L)
#define u64_literal(x) (x##UL)

#define I8_MIN -128
#define I8_MAX 127
#define U8_MAX 255
#define I16_MIN -32768
#define I16_MAX 32767
#define U16_MAX 65535
#define I32_MIN (-0x7FFFFFFF - 1)
#define I32_MAX (0x7FFFFFFF)
#define U32_MAX (0xFFFFFFFFU)
#define I64_MIN (-i64_literal(0x7FFFFFFFFFFFFFFF) - 1)
#define I64_MAX i64_literal(0x7FFFFFFFFFFFFFFF)
#define U64_MAX u64_literal(0xFFFFFFFFFFFFFFFF)

#define PRINT_I8 "%d"
#define PRINT_I16 "%d"
#define PRINT_I32 "%d"
#define PRINT_I64 "%lld"
#define PRINT_U8 "%u"
#define PRINT_U16 "%u"
#define PRINT_U32 "%u"
#define PRINT_U64 "%llu"
#define PRINT_STR "%s"

#define _base_cast_const_type(TOTYPE,FROMTYPE,X) ((__extension__(union {FROMTYPE _q; TOTYPE _nq;})(X))._nq)
#define base_cast_const(TYPE,X) _base_cast_const_type(TYPE, const TYPE, (X))

#define base_likely(x) __builtin_expect((x) != 0, 1)
#define base_unlikely(x) __builtin_expect((x) != 0, 0)

#define base_must_check __attribute__((warn_unused_result))
#define base_no_return __attribute__((noreturn)) void
#define base_no_null __attribute__((nonnull))
#define base_private static 

base_no_return _dbg_assert_failed(
    const ch_t *expr, const ch_t *file, u32_t line_no);

#define dbg_assert(expr)                                                       \
    do {                                                                       \
        if (base_unlikely(!(expr))) {                                          \
            _dbg_assert_failed(#expr, __FILE__, (u32_t)__LINE__);                \
        }                                                                      \
    } while (0)

#ifdef BASE_DBG_ENABLED
#define dbg_assert_debug dbg_assert(expr)
#else
#define dbg_assert_debug
#endif
#define dbg_assert_unreachable() do { dbg_assert(false); __builtin_unreachable(); } while(0)

#ifdef __cplusplus
}
#endif

#endif
