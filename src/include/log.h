#ifndef ___LOG
#define ___LOG

#include "base.h"

void log_info_str_len(const ch_t *str, usz_t len);
void log_info_str(const ch_t *str);
void log_info_uint(u64_t uval);
void log_info_str_line_len(const ch_t *str, usz_t len);
void log_info_str_line(const ch_t *str);
 
#endif
