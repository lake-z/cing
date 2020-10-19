#include <cpuid.h>
#include "drivers_screen.h"
#include "kernel_env.h"
#include "kernel_panic.h"

#define _CPU_VENDOR_LEN 16
base_private ch_t _cpu_vendor[_CPU_VENDOR_LEN];

void env_init_cpu_info(void)
{
/*
  base_private const usz_t _MSG_LEN = 128;
  ch_t msg[_MSG_LEN];
  usz_t msg_len;
  const ch_t *msg_part;
*/
  u32_t eax;
  u32_t ebx;
  u32_t ecx;
  u32_t edx;
  i32_t ok;
  
  /* unsigned int __get_cpuid_max (unsigned int __ext, unsigned int *__sig) 
   *
   * Return highest supported input value for cpuid instruction.  
   * ext can be either 0x0 or 0x80000000 to return highest supported value for
   * basic or extended cpuid information.  
   * Function returns 0 if cpuid is not supported or whatever cpuid returns in 
   * eax register.  
   * If sig pointer is non-null, then first four bytes of the signature
   * (as found in ebx register) are returned in location pointed by sig. */
  eax = __get_cpuid_max(0x0, NULL);
  kernel_assert(eax != 0);  

  /* int __get_cpuid (unsigned int __leaf, unsigned int *__eax, 
   *    unsigned int *__ebx, unsigned int *__ecx, unsigned int *__edx) 
   * 
   * Return cpuid data for requested cpuid leaf, as found in returned eax, ebx, 
   * ecx and edx registers. 
   * The function checks if cpuid is supported and returns 1 for valid cpuid 
   * information or 0 for unsupported cpuid leaf.  
   * All pointers are required to be non-null. */

  /* CPUID leaf EAX=0: Highest Function Parameter and Manufacturer ID */
  ok = __get_cpuid (0x0, &eax, &ebx, &ecx, &edx);  
  kernel_assert(ok == 1);
  *(u32_t *)_cpu_vendor = ebx; 
  *(u32_t *)(_cpu_vendor + 4) = edx; 
  *(u32_t *)(_cpu_vendor + 8) = ecx; 
  _cpu_vendor[12] = '\0';
  screen_write_str(_cpu_vendor, SCREEN_COLOR_LIGHT_RED, SCREEN_COLOR_BLACK, 0, 
    0);
}
