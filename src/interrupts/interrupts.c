#include "interrupts.h"
#include "containers_string.h"
#include "drivers_port.h"
#include "drivers_screen.h"
#include "kernel_panic.h"

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

extern void isr37(void);
extern void isr38(void);
extern void isr39(void);
extern void isr40(void);
extern void isr41(void);
extern void isr42(void);
extern void isr43(void);
extern void isr44(void);
extern void isr45(void);
extern void isr46(void);
extern void isr47(void);
extern void isr48(void);
extern void isr49(void);
extern void isr50(void);
extern void isr51(void);
extern void isr52(void);
extern void isr53(void);
extern void isr54(void);
extern void isr55(void);
extern void isr56(void);
extern void isr57(void);
extern void isr58(void);
extern void isr59(void);
extern void isr60(void);
extern void isr61(void);
extern void isr62(void);
extern void isr63(void);
extern void isr64(void);
extern void isr65(void);
extern void isr66(void);
extern void isr67(void);
extern void isr68(void);
extern void isr69(void);
extern void isr70(void);
extern void isr71(void);
extern void isr72(void);
extern void isr73(void);
extern void isr74(void);
extern void isr75(void);
extern void isr76(void);
extern void isr77(void);
extern void isr78(void);
extern void isr79(void);
extern void isr80(void);
extern void isr81(void);
extern void isr82(void);
extern void isr83(void);
extern void isr84(void);
extern void isr85(void);
extern void isr86(void);
extern void isr87(void);
extern void isr88(void);
extern void isr89(void);
extern void isr90(void);
extern void isr91(void);
extern void isr92(void);
extern void isr93(void);
extern void isr94(void);
extern void isr95(void);
extern void isr96(void);
extern void isr97(void);
extern void isr98(void);
extern void isr99(void);
extern void isr100(void);
extern void isr101(void);
extern void isr102(void);
extern void isr103(void);
extern void isr104(void);
extern void isr105(void);
extern void isr106(void);
extern void isr107(void);
extern void isr108(void);
extern void isr109(void);
extern void isr110(void);
extern void isr111(void);
extern void isr112(void);
extern void isr113(void);
extern void isr114(void);
extern void isr115(void);
extern void isr116(void);
extern void isr117(void);
extern void isr118(void);
extern void isr119(void);
extern void isr120(void);
extern void isr121(void);
extern void isr122(void);
extern void isr123(void);
extern void isr124(void);
extern void isr125(void);
extern void isr126(void);
extern void isr127(void);
extern void isr128(void);
extern void isr129(void);
extern void isr130(void);
extern void isr131(void);
extern void isr132(void);
extern void isr133(void);
extern void isr134(void);
extern void isr135(void);
extern void isr136(void);
extern void isr137(void);
extern void isr138(void);
extern void isr139(void);
extern void isr140(void);
extern void isr141(void);
extern void isr142(void);
extern void isr143(void);
extern void isr144(void);
extern void isr145(void);
extern void isr146(void);
extern void isr147(void);
extern void isr148(void);
extern void isr149(void);
extern void isr150(void);
extern void isr151(void);
extern void isr152(void);
extern void isr153(void);
extern void isr154(void);
extern void isr155(void);
extern void isr156(void);
extern void isr157(void);
extern void isr158(void);
extern void isr159(void);
extern void isr160(void);
extern void isr161(void);
extern void isr162(void);
extern void isr163(void);
extern void isr164(void);
extern void isr165(void);
extern void isr166(void);
extern void isr167(void);
extern void isr168(void);
extern void isr169(void);
extern void isr170(void);
extern void isr171(void);
extern void isr172(void);
extern void isr173(void);
extern void isr174(void);
extern void isr175(void);
extern void isr176(void);
extern void isr177(void);
extern void isr178(void);
extern void isr179(void);
extern void isr180(void);
extern void isr181(void);
extern void isr182(void);
extern void isr183(void);
extern void isr184(void);
extern void isr185(void);
extern void isr186(void);
extern void isr187(void);
extern void isr188(void);
extern void isr189(void);
extern void isr190(void);
extern void isr191(void);
extern void isr192(void);
extern void isr193(void);
extern void isr194(void);
extern void isr195(void);
extern void isr196(void);
extern void isr197(void);
extern void isr198(void);
extern void isr199(void);
extern void isr200(void);
extern void isr201(void);
extern void isr202(void);
extern void isr203(void);
extern void isr204(void);
extern void isr205(void);
extern void isr206(void);
extern void isr207(void);
extern void isr208(void);
extern void isr209(void);
extern void isr210(void);
extern void isr211(void);
extern void isr212(void);
extern void isr213(void);
extern void isr214(void);
extern void isr215(void);
extern void isr216(void);
extern void isr217(void);
extern void isr218(void);
extern void isr219(void);
extern void isr220(void);
extern void isr221(void);
extern void isr222(void);
extern void isr223(void);
extern void isr224(void);
extern void isr225(void);
extern void isr226(void);
extern void isr227(void);
extern void isr228(void);
extern void isr229(void);
extern void isr230(void);
extern void isr231(void);
extern void isr232(void);
extern void isr233(void);
extern void isr234(void);
extern void isr235(void);
extern void isr236(void);
extern void isr237(void);
extern void isr238(void);
extern void isr239(void);
extern void isr240(void);
extern void isr241(void);
extern void isr242(void);
extern void isr243(void);
extern void isr244(void);
extern void isr245(void);
extern void isr246(void);
extern void isr247(void);
extern void isr248(void);
extern void isr249(void);
extern void isr250(void);
extern void isr251(void);
extern void isr252(void);
extern void isr253(void);
extern void isr254(void);
extern void isr255(void);

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

  _idt_gate_encode(37, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr37);
  _idt_gate_encode(38, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr38);
  _idt_gate_encode(39, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr39);
  _idt_gate_encode(40, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr40);
  _idt_gate_encode(41, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr41);
  _idt_gate_encode(42, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr42);
  _idt_gate_encode(43, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr43);
  _idt_gate_encode(44, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr44);
  _idt_gate_encode(45, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr45);
  _idt_gate_encode(46, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr46);
  _idt_gate_encode(47, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr47);
  _idt_gate_encode(48, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr48);
  _idt_gate_encode(49, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr49);
  _idt_gate_encode(50, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr50);
  _idt_gate_encode(51, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr51);
  _idt_gate_encode(52, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr52);
  _idt_gate_encode(53, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr53);
  _idt_gate_encode(54, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr54);
  _idt_gate_encode(55, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr55);
  _idt_gate_encode(56, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr56);
  _idt_gate_encode(57, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr57);
  _idt_gate_encode(58, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr58);
  _idt_gate_encode(59, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr59);
  _idt_gate_encode(60, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr60);
  _idt_gate_encode(61, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr61);
  _idt_gate_encode(62, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr62);
  _idt_gate_encode(63, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr63);
  _idt_gate_encode(64, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr64);
  _idt_gate_encode(65, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr65);
  _idt_gate_encode(66, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr66);
  _idt_gate_encode(67, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr67);
  _idt_gate_encode(68, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr68);
  _idt_gate_encode(69, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr69);
  _idt_gate_encode(70, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr70);
  _idt_gate_encode(71, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr71);
  _idt_gate_encode(72, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr72);
  _idt_gate_encode(73, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr73);
  _idt_gate_encode(74, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr74);
  _idt_gate_encode(75, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr75);
  _idt_gate_encode(76, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr76);
  _idt_gate_encode(77, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr77);
  _idt_gate_encode(78, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr78);
  _idt_gate_encode(79, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr79);
  _idt_gate_encode(80, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr80);
  _idt_gate_encode(81, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr81);
  _idt_gate_encode(82, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr82);
  _idt_gate_encode(83, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr83);
  _idt_gate_encode(84, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr84);
  _idt_gate_encode(85, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr85);
  _idt_gate_encode(86, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr86);
  _idt_gate_encode(87, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr87);
  _idt_gate_encode(88, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr88);
  _idt_gate_encode(89, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr89);
  _idt_gate_encode(90, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr90);
  _idt_gate_encode(91, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr91);
  _idt_gate_encode(92, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr92);
  _idt_gate_encode(93, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr93);
  _idt_gate_encode(94, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr94);
  _idt_gate_encode(95, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr95);
  _idt_gate_encode(96, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr96);
  _idt_gate_encode(97, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr97);
  _idt_gate_encode(98, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr98);
  _idt_gate_encode(99, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr99);
  _idt_gate_encode(100, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr100);
  _idt_gate_encode(101, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr101);
  _idt_gate_encode(102, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr102);
  _idt_gate_encode(103, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr103);
  _idt_gate_encode(104, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr104);
  _idt_gate_encode(105, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr105);
  _idt_gate_encode(106, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr106);
  _idt_gate_encode(107, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr107);
  _idt_gate_encode(108, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr108);
  _idt_gate_encode(109, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr109);
  _idt_gate_encode(110, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr110);
  _idt_gate_encode(111, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr111);
  _idt_gate_encode(112, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr112);
  _idt_gate_encode(113, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr113);
  _idt_gate_encode(114, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr114);
  _idt_gate_encode(115, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr115);
  _idt_gate_encode(116, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr116);
  _idt_gate_encode(117, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr117);
  _idt_gate_encode(118, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr118);
  _idt_gate_encode(119, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr119);
  _idt_gate_encode(120, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr120);
  _idt_gate_encode(121, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr121);
  _idt_gate_encode(122, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr122);
  _idt_gate_encode(123, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr123);
  _idt_gate_encode(124, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr124);
  _idt_gate_encode(125, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr125);
  _idt_gate_encode(126, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr126);
  _idt_gate_encode(127, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr127);
  _idt_gate_encode(128, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr128);
  _idt_gate_encode(129, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr129);
  _idt_gate_encode(130, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr130);
  _idt_gate_encode(131, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr131);
  _idt_gate_encode(132, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr132);
  _idt_gate_encode(133, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr133);
  _idt_gate_encode(134, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr134);
  _idt_gate_encode(135, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr135);
  _idt_gate_encode(136, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr136);
  _idt_gate_encode(137, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr137);
  _idt_gate_encode(138, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr138);
  _idt_gate_encode(139, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr139);
  _idt_gate_encode(140, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr140);
  _idt_gate_encode(141, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr141);
  _idt_gate_encode(142, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr142);
  _idt_gate_encode(143, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr143);
  _idt_gate_encode(144, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr144);
  _idt_gate_encode(145, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr145);
  _idt_gate_encode(146, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr146);
  _idt_gate_encode(147, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr147);
  _idt_gate_encode(148, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr148);
  _idt_gate_encode(149, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr149);
  _idt_gate_encode(150, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr150);
  _idt_gate_encode(151, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr151);
  _idt_gate_encode(152, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr152);
  _idt_gate_encode(153, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr153);
  _idt_gate_encode(154, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr154);
  _idt_gate_encode(155, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr155);
  _idt_gate_encode(156, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr156);
  _idt_gate_encode(157, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr157);
  _idt_gate_encode(158, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr158);
  _idt_gate_encode(159, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr159);
  _idt_gate_encode(160, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr160);
  _idt_gate_encode(161, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr161);
  _idt_gate_encode(162, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr162);
  _idt_gate_encode(163, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr163);
  _idt_gate_encode(164, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr164);
  _idt_gate_encode(165, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr165);
  _idt_gate_encode(166, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr166);
  _idt_gate_encode(167, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr167);
  _idt_gate_encode(168, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr168);
  _idt_gate_encode(169, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr169);
  _idt_gate_encode(170, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr170);
  _idt_gate_encode(171, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr171);
  _idt_gate_encode(172, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr172);
  _idt_gate_encode(173, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr173);
  _idt_gate_encode(174, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr174);
  _idt_gate_encode(175, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr175);
  _idt_gate_encode(176, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr176);
  _idt_gate_encode(177, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr177);
  _idt_gate_encode(178, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr178);
  _idt_gate_encode(179, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr179);
  _idt_gate_encode(180, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr180);
  _idt_gate_encode(181, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr181);
  _idt_gate_encode(182, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr182);
  _idt_gate_encode(183, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr183);
  _idt_gate_encode(184, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr184);
  _idt_gate_encode(185, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr185);
  _idt_gate_encode(186, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr186);
  _idt_gate_encode(187, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr187);
  _idt_gate_encode(188, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr188);
  _idt_gate_encode(189, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr189);
  _idt_gate_encode(190, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr190);
  _idt_gate_encode(191, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr191);
  _idt_gate_encode(192, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr192);
  _idt_gate_encode(193, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr193);
  _idt_gate_encode(194, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr194);
  _idt_gate_encode(195, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr195);
  _idt_gate_encode(196, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr196);
  _idt_gate_encode(197, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr197);
  _idt_gate_encode(198, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr198);
  _idt_gate_encode(199, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr199);
  _idt_gate_encode(200, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr200);
  _idt_gate_encode(201, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr201);
  _idt_gate_encode(202, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr202);
  _idt_gate_encode(203, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr203);
  _idt_gate_encode(204, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr204);
  _idt_gate_encode(205, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr205);
  _idt_gate_encode(206, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr206);
  _idt_gate_encode(207, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr207);
  _idt_gate_encode(208, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr208);
  _idt_gate_encode(209, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr209);
  _idt_gate_encode(210, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr210);
  _idt_gate_encode(211, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr211);
  _idt_gate_encode(212, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr212);
  _idt_gate_encode(213, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr213);
  _idt_gate_encode(214, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr214);
  _idt_gate_encode(215, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr215);
  _idt_gate_encode(216, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr216);
  _idt_gate_encode(217, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr217);
  _idt_gate_encode(218, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr218);
  _idt_gate_encode(219, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr219);
  _idt_gate_encode(220, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr220);
  _idt_gate_encode(221, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr221);
  _idt_gate_encode(222, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr222);
  _idt_gate_encode(223, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr223);
  _idt_gate_encode(224, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr224);
  _idt_gate_encode(225, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr225);
  _idt_gate_encode(226, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr226);
  _idt_gate_encode(227, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr227);
  _idt_gate_encode(228, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr228);
  _idt_gate_encode(229, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr229);
  _idt_gate_encode(230, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr230);
  _idt_gate_encode(231, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr231);
  _idt_gate_encode(232, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr232);
  _idt_gate_encode(233, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr233);
  _idt_gate_encode(234, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr234);
  _idt_gate_encode(235, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr235);
  _idt_gate_encode(236, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr236);
  _idt_gate_encode(237, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr237);
  _idt_gate_encode(238, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr238);
  _idt_gate_encode(239, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr239);
  _idt_gate_encode(240, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr240);
  _idt_gate_encode(241, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr241);
  _idt_gate_encode(242, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr242);
  _idt_gate_encode(243, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr243);
  _idt_gate_encode(244, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr244);
  _idt_gate_encode(245, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr245);
  _idt_gate_encode(246, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr246);
  _idt_gate_encode(247, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr247);
  _idt_gate_encode(248, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr248);
  _idt_gate_encode(249, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr249);
  _idt_gate_encode(250, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr250);
  _idt_gate_encode(251, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr251);
  _idt_gate_encode(252, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr252);
  _idt_gate_encode(253, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr253);
  _idt_gate_encode(254, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr254);
  _idt_gate_encode(255, IDT_GATE_TYPE_INTERRUPT, (uptr_t)isr255);
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
  const usz_t MSG_CAP = 128;
  ch_t msg[MSG_CAP];
  const ch_t *msg_part;
  usz_t msg_len;

  kernel_assert(id < INTR_ID_MAX);
  iid = (intr_id_t)id;

  msg_len = 0;
  msg_part = "TODO: intr_isr_handler, id = ";
  msg_len +=
      str_buf_marshal_str(msg, msg_len, MSG_CAP, msg_part, str_len(msg_part));
  msg_len += str_buf_marshal_uint(msg, msg_len, MSG_CAP, iid);
  msg_len += str_buf_marshal_terminator(msg, msg_len, MSG_CAP);
  kernel_panic(msg);

  (void)(stack_addr);
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
