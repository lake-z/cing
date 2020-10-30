#include "interrupts.h"
#include "containers_string.h"
#include "drivers_screen.h"
#include "kernel_panic.h"
#include "kernel_port.h"

/* Forwarded declarations */
typedef enum { IDT_GATE_TYPE_INTERRUPT, IDT_GATE_TYPE_TRAP } idt_gate_type_t;

extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);
extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);

#define IDT_GATE_LEN 16
#define IDT_GATE_COUNT 256

/*
 * Code segement initialiezd as the second entey of GDT.
 *
 * Some notes about GDT:
 *
 * A GDT always starts with a 0-entry and contains an arbitrary number of
 * segment entries afterwards.
 *
 * The first entry of the GDT is a null segment selector(that is, a segment
 * selector with an index of 0 and the TI flag set to 0). The processor
 * generates an exception when a segment register holding a null selector is
 * used to access memory. The processor does not generate an exception when a
 * segment register (other than the CS or SS registers) is loaded with a null
 * selector. A null selector can be used to initialize unused segment registers
 * Loading the CS or SS register with a null segment selector causes a
 * general-protection exception (#GP) to be generated.
 */
base_private const u16_t GDT_CODE_SEGMENT_OFFSET = 0x08;

/* Interrupt Descriptor Table, has 256 gates, and 18 bytes len each */
base_private byte_t _idt[IDT_GATE_LEN * IDT_GATE_COUNT];

/* 1 to 1 mapping from IDT gate to interrupt handler */
base_private intr_handler_cb handlers[IDT_GATE_COUNT];

/*
 ******************************************************************************
                    ____________                          ____________
Real Time Clock -> |            |   Timer -------------> |            |
ACPI ------------> |            |   Keyboard-----------> |            |
Available -------> | Secondary  |----------------------> | Primary    |     C
Available -------> | Interrupt  |   Serial Port 2 -----> | Interrupt  |---> P
Mouse -----------> | Controller |   Serial Port 1 -----> | Controller |     U
Co-Processor ----> |            |   Parallel Port 2/3 -> |            |
Primary ATA -----> |            |   Floppy disk -------> |            |
Secondary ATA ---> |____________|   Parallel Port 1----> |____________|

 * ****************************************************************************
 */
/* The default configuration of the PICs is not usable, because it sends
  * interrupt vector numbers in the range 0–15 to the CPU. These numbers are
  * already occupied by CPU exceptions.
  * Mapping IRQ 0..15 to interrupt handler index 32..32+15 */
base_private const byte_t IRQ_HANDLER_BASE = 32;

base_private const byte_t *_idt_gate_encode(
    usz_t gate_idx, idt_gate_type_t type, uptr_t handler)
{
  byte_t *gate;

  kernel_assert(gate_idx < IDT_GATE_COUNT);
  gate = _idt + gate_idx * IDT_GATE_LEN;

  /* Field offset1 */
  *(u16_t *)gate = (u16_t)handler;

  /* Field selector:
   * a 16 bit value and must point to a valid descriptor in GDT. */
  *(u16_t *)(gate + 2) = GDT_CODE_SEGMENT_OFFSET;

  /* Two bytes options following:
   * bits 0 ~ 2:
   *  Interrupt Stack Table (IST) Index,
   *    0, Don't switch stacks;
   *    1-7, Switch to the n-th stack in the Interrupt Stack Table when this
   *      handler is called
   * bits 3 ~ 7: Reserved
   * bit 8: 0, Interrupt Gate, 1, Trap Gate; If this bit is 0, interrupts are
   *    disabled when this handler is called.
   * bits 9 ~ 11: Must be 1
   * bit 12: Must be 0
   * bits 13 ~ 14: Descriptor Privilege Level (DPL), The minimal privilege
   *    level required for calling this handler.
   * bit 15: Present, Raise a double fault if this bit is unset.
   */
  gate[4] = 0;
  if (type == IDT_GATE_TYPE_INTERRUPT) {
    gate[5] = 0x8f;
  } else if (type == IDT_GATE_TYPE_TRAP) {
    gate[5] = 0x8e;
  } else {
    kernel_panic("Invalid IDT gate type");
  }

  /* Field offset2 */
  *(u16_t *)(gate + 6) = (u16_t)(handler >> 16);

  /* Field offset3 */
  *(u32_t *)(gate + 8) = (u32_t)(handler >> 32);

  /* Field reserved */
  *(u32_t *)(gate + 12) = 0;

  return gate;
}

base_private void _intr_init_pic_8259(void)
{
  /* Start initialization */
  port_write_byte(PORT_NO_PIC_MASTER_CMD, 0x11);
  port_write_byte(PORT_NO_PIC_SLAVE_CMD, 0x11);

  /* Set IRQ base numbers for each PIC
   * ISR handler offset for master PIC become
   * IRQ_HANDLER_BASE..IRQ_HANDLER_BASE+7
   * same for slave PIC: IRQ_HANDLER_BASE+8..IRQ_HANDLER_BASE+15 */
  port_write_byte(PORT_NO_PIC_MASTER_DATA, IRQ_HANDLER_BASE);
  port_write_byte(PORT_NO_PIC_SLAVE_DATA, (byte_t)(IRQ_HANDLER_BASE + 8));

  /* Tell master PIC that there is a slave PIC at IRQ2 (0000 0100) */
  port_write_byte(PORT_NO_PIC_MASTER_DATA, 0x04);
  /* Tell slave PIC its cascade identity (0000 0010) */
  port_write_byte(PORT_NO_PIC_SLAVE_DATA, 0x02);

  /* Finish initialization */
  port_write_byte(PORT_NO_PIC_MASTER_DATA, 0x01);
  port_write_byte(PORT_NO_PIC_SLAVE_DATA, 0x01);

  /* Enable all IRQs */
  port_write_byte(PORT_NO_PIC_MASTER_DATA, 0x00);
  port_write_byte(PORT_NO_PIC_SLAVE_DATA, 0x00);
}

base_private void _intr_init_idt(void)
{
  _idt_gate_encode(0, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr0);
  _idt_gate_encode(1, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr1);
  _idt_gate_encode(2, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr2);
  _idt_gate_encode(3, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr3);
  _idt_gate_encode(4, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr4);
  _idt_gate_encode(5, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr5);
  _idt_gate_encode(6, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr6);
  _idt_gate_encode(7, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr7);
  _idt_gate_encode(8, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr8);
  _idt_gate_encode(9, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr9);
  _idt_gate_encode(10, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr10);
  _idt_gate_encode(11, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr11);
  _idt_gate_encode(12, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr12);
  _idt_gate_encode(13, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr13);
  _idt_gate_encode(14, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr14);
  _idt_gate_encode(15, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr15);
  _idt_gate_encode(16, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr16);
  _idt_gate_encode(17, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr17);
  _idt_gate_encode(18, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr18);
  _idt_gate_encode(19, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr19);
  _idt_gate_encode(20, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr20);
  _idt_gate_encode(21, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr21);
  _idt_gate_encode(22, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr22);
  _idt_gate_encode(23, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr23);
  _idt_gate_encode(24, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr24);
  _idt_gate_encode(25, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr25);
  _idt_gate_encode(26, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr26);
  _idt_gate_encode(27, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr27);
  _idt_gate_encode(28, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr28);
  _idt_gate_encode(29, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr29);
  _idt_gate_encode(30, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr30);
  _idt_gate_encode(31, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr31);
  _idt_gate_encode(IRQ_HANDLER_BASE, IDT_GATE_TYPE_INTERRUPT, (uptr_t)irq0);
  _idt_gate_encode(
      (byte_t)(IRQ_HANDLER_BASE + 1), IDT_GATE_TYPE_INTERRUPT, (uptr_t)irq1);
  _idt_gate_encode(
      (byte_t)(IRQ_HANDLER_BASE + 2), IDT_GATE_TYPE_INTERRUPT, (uptr_t)irq2);
  _idt_gate_encode(
      (byte_t)(IRQ_HANDLER_BASE + 3), IDT_GATE_TYPE_INTERRUPT, (uptr_t)irq3);
  _idt_gate_encode(
      (byte_t)(IRQ_HANDLER_BASE + 4), IDT_GATE_TYPE_INTERRUPT, (uptr_t)irq4);
}

base_private void _intr_load_idt_register(void)
{
  byte_t idt_register[10];

  /* Field limit: maximum addressable byte in table, 2 bytes. */
  *(u16_t *)idt_register = (IDT_GATE_LEN * IDT_GATE_COUNT - 1);

  /* Field offset: Linear address of IDT, 8 bytes. */
  *(vptr_t *)(idt_register + 2) = (vptr_t *)&_idt;

  __asm__("lidt %0" : : "m"(idt_register));
}

void intr_init(void)
{
  _intr_init_idt();
  _intr_load_idt_register();
  _intr_init_pic_8259();

  for (usz_t i = 0; i < IDT_GATE_COUNT; i++) {
    handlers[i] = NULL;
  }
}

void intr_irq_enable(void)
{
  __asm__("sti");
}

void intr_irq_disable(void)
{
  __asm__("cli");
}

void intr_isr_handler(u64_t id, uptr_t stack_addr)
{
  intr_id_t iid;

  kernel_assert(id < INTR_ID_MAX);
  iid = (intr_id_t)id;

  kernel_panic("TODO: intr_isr_handler");
  (void)(iid + stack_addr);
}

void intr_irq_handler(u64_t id, uptr_t stack_addr)
{
  intr_id_t iid;
  intr_parameters_t *paras;
  intr_handler_cb hand;
  const usz_t MSG_CAP = 128;
  ch_t msg[MSG_CAP];
  const ch_t *msg_part;
  usz_t msg_len;

  /* End Of Interrupt (EOI, code 0x20) command is issued to the PIC chips at 
   * the end of an IRQ-based interrupt routine. If the IRQ came from the Master 
   * PIC, it is sufficient to issue this command only to the Master PIC; 
   * however if the IRQ came from the Slave PIC, it is necessary to issue the 
   * command to both PIC chips. */
  if (id >= 40) {
    port_write_byte(PORT_NO_PIC_SLAVE_CMD, 0x20);
  }
  port_write_byte(PORT_NO_PIC_MASTER_CMD, 0x20);

  kernel_assert(id < INTR_ID_MAX);
  iid = (intr_id_t)id;
  hand = handlers[iid];

  if (hand == NULL) {
    msg_len = 0;
    msg_part = "Unhandled IRQ, id: ";
    msg_len =
        str_buf_marshal_str(msg, msg_len, MSG_CAP, msg_part, str_len(msg_part));
    msg_len += str_buf_marshal_uint(msg, msg_len, MSG_CAP, (u64_t)iid);
    msg_len += str_buf_marshal_terminator(msg, msg_len, MSG_CAP);
    kernel_panic(msg);
  } else {
    paras = (intr_parameters_t *)stack_addr;
    (*hand)(id, paras);
  }
}

void intr_handler_register(intr_id_t id, intr_handler_cb handler)
{
  kernel_assert(id < INTR_ID_MAX);
  kernel_assert(handlers[id] == NULL);
  kernel_assert(handler != NULL);
  handlers[id] = handler;
}
