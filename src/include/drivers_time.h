#ifndef ___DRIVERS_TIME
#define ___DRIVERS_TIME

#include "base.h"
#include "interrupts.h"

void time_irq_handler(intr_id_t id, intr_parameters_t *para);
void time_init(void);

#endif
