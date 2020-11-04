#include "cpu.h"

u64_t cpu_read_cr2(void)
{
  u64_t value;

  __asm__("mov %%cr2, %0" : "=r"(value) : /* no input */);

  return value;
}

void cpu_write_cr3(u64_t value)
{
  __asm__("mov %0, %%cr3" : /* no output */ : "r"(value));
}

u64_t cpu_read_cr3(void)
{
  u64_t value;

  __asm__("mov %%cr3, %0" : "=r"(value) : /* no input */);

  return value;
}
