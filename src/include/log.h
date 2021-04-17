#ifndef ___LOG
#define ___LOG

#include "base.h"

typedef enum {
  LOG_LEVEL_SELF_TEST = 0,
  LOG_LEVEL_DEBUG = 1,
  LOG_LEVEL_INFO = 2,
  LOG_LEVEL_WARN = 3,
  LOG_LEVEL_ERROR = 4,
  LOG_LEVEL_FATAL = 5,
} log_level_t;

void log_enable_video_write(void);
//void log_uint_of_size(log_level_t lv, u64_t uval);
void _log_builtin_test_pass(
    const ch_t *test_name, const ch_t *file, usz_t line);
void _log_line_format_v(
    log_level_t lv, const ch_t *file, usz_t line, const ch_t *format, ...)
    base_check_format(4, 5);

#define log_builtin_test_pass()                                                \
  do {                                                                         \
    _log_builtin_test_pass(__FUNCTION__, __FILE__, __LINE__);                  \
  } while (0)

#define log_line_format(level, format, args...)                                \
  do {                                                                         \
    _log_line_format_v(level, __FILE__, __LINE__, format, ##args);             \
  } while (0)

#endif
