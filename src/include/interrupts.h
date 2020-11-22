#ifndef ___INTERRUPTS
#define ___INTERRUPTS

#include "base.h"

/* Actually a pointer to the stack, CPU will first push related registers as
 * on stack as arguments to ISR, see:
 * https://os.phil-opp.com/cpu-exceptions/#the-interrupt-stack-frame
 *
 * And then, all registers are pushed onto stack and to be restored later by
 * isr_common_stub or irq_common_stub.
 *
 * And finally note that stack grows downward. */
typedef byte_t intr_parameters_t;
typedef enum {
  INTR_ID_EX_FAULT_DE = 0,
  INTR_ID_EX_FAULT_TRAP_DB = 1,
  INTR_ID_EX_INTERRUPT_NMI = 2,
  INTR_ID_EX_TRAP_BP = 3,
  INTR_ID_EX_TRAP_OF = 4,
  INTR_ID_EX_FAULT_BR = 5,
  INTR_ID_EX_FAULT_UD = 6,
  INTR_ID_EX_FAULT_NM = 7,
  INTR_ID_EX_ABORT_DF = 8,
  /* 9 is not deprecated. */
  INTR_ID_EX_FAULT_TS = 10,
  INTR_ID_EX_FAULT_NP = 11,
  INTR_ID_EX_FAULT_SS = 12,
  INTR_ID_EX_FAULT_GP = 13,
  INTR_ID_EX_FAULT_PF = 14,
  /* 15 is reserved. */
  INTR_ID_EX_FAULT_MF = 16,
  INTR_ID_EX_FAULT_AC = 17,
  INTR_ID_EX_ABORT_MC = 18,
  INTR_ID_EX_FAULT_XF = 19,
  INTR_ID_EX_FAULT_VE = 20,
  /* 21..29 is reserved. */
  INTR_ID_EX_SX = 30,
  /* 31 is reserved. */
  INTR_ID_IRQ_TIME = 32,
  INTR_ID_IRQ_KEYBOARD = 33,
  /* 34 Cascade (used internally by the two PICs. never raised) */
  INTR_ID_IRQ_COM2 = 35,
  INTR_ID_IRQ_COM1 = 36,
  INTR_ID_IRQ_LPT2 = 37,
  INTR_ID_IRQ_FLOPPY = 38,
  INTR_ID_IRQ_LPT1 = 39, /* Unreliable "spurious" interrupt (usually) */
  INTR_ID_MAX            /* End token, not a valid interrupt ID */
} intr_id_t;
typedef void (*intr_handler_cb)(intr_id_t id, intr_parameters_t *para);

/* Initialize interrupt module, after this initialization is finished, IRQs
 * will be in disabled state, enable with intr_irq_enable(). */
void intr_init(void);

void intr_irq_enable(void);
void intr_irq_disable(void);
void intr_isr_handler(u64_t id, uptr_t stack_addr);
void intr_irq_handler(u64_t id, uptr_t stack_addr);
void intr_handler_register(intr_id_t id, intr_handler_cb handler);

#endif
