#ifndef ___LOG
#define ___LOG

#include "base.h"

void log_info_ln_len(const ch_t *str, usz_t len);
void log_info_ln(const ch_t *str);
void log_info_len(const ch_t *str, usz_t len);
void log_info(const ch_t *str);
void log_info_uint(u64_t uval);

#endif
