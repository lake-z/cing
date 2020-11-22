#ifndef ___DRIVERS_SERIAL
#define ___DRIVERS_SERIAL

#include "base.h"

void serial_init(void);
void serial_write_str(const ch_t *str, usz_t len);

#endif
