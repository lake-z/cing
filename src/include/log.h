#ifndef ___LOG
#define ___LOG

#include "base.h"

typedef enum {
  LOG_LEVEL_DEBUG = 0, 
  LOG_LEVEL_INFO = 1,
  LOG_LEVEL_WARN = 2,
  LOG_LEVEL_ERROR = 3,
  LOG_LEVEL_FATAL = 4,
} log_level_t;

void log_str_len(log_level_t lv, const ch_t *str, usz_t len);
void log_str(log_level_t lv, const ch_t *str);
void log_uint(log_level_t lv, u64_t uval);
void log_uint_of_size(log_level_t lv, u64_t uval);
void log_line_start(log_level_t lv);
void log_line_end(log_level_t lv);

#endif
