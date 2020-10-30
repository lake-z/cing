#include "drivers_time.h"

void time_irq_handler(intr_id_t id, intr_parameters_t *para)
{
  (void)(id + para);
  /* Do nothing now. */
}

void time_init(void)
{
  intr_handler_register(INTR_ID_IRQ_TIME, time_irq_handler);
}
