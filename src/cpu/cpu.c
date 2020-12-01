#include "cpu.h"

u64_t cpu_read_cr2(void)
{
  u64_t value;
  __asm__("movq %%cr2, %0" : "=r"(value) : /* no input */);
  return value;
}

void cpu_write_cr3(u64_t value)
{
  __asm__("movq %0, %%cr3" : /* no output */ : "r"(value));
}

u64_t cpu_read_cr3(void)
{
  u64_t value;
  __asm__("movq %%cr3, %0" : "=r"(value) : /* no input */);
  return value;
}

u64_t cpu_read_rbp(void)
{
  u64_t value;
  __asm__("movq %%rbp, %0" : "=r"(value) : /* no input */);
  return value;
}

void cpu_write_rbp(u64_t value)
{
  __asm__("movq %0, %%rbp" : /* no output */ : "r"(value));
}

u64_t cpu_read_rsp(void)
{
  u64_t value;
  __asm__("movq %%rsp, %0" : "=r"(value) : /* no input */);
  return value;
}

void cpu_write_rsp(u64_t value)
{
  __asm__("movq %0, %%rsp" : /* no output */ : "r"(value));
}
