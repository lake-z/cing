#ifndef ___CPU
#define ___CPU

#include "base.h"

u64_t cpu_read_cr2(void);
void cpu_write_cr3(u64_t value);
u64_t cpu_read_cr3(void);
u64_t cpu_read_rbp(void);
void cpu_write_rbp(u64_t value);
u64_t cpu_read_rsp(void);
void cpu_write_rsp(u64_t value);

#endif
