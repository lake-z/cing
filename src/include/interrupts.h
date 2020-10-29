#ifndef ___INTERRUPTS
#define ___INTERRUPTS

#include "base.h"

void intr_init(void);
void intr_irq_enable(void);
void intr_irq_disable(void);
void intr_isr_handler(u64_t id, uptr_t stack_addr);
void intr_irq_handler(u64_t id, uptr_t stack_addr);

#endif
