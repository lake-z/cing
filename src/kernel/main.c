#include "base.h"
#include "drivers_screen.h"
#include "kernel_panic.h"
#include <string.h> // TODO: Remove this later

#define FB_COMMAND_PORT      0x3D4
#define FB_DATA_PORT         0x3D5
#define FB_HIGH_BYTE_COMMAND 14
#define FB_LOW_BYTE_COMMAND  15

void kmain(uint64_t addr)
{
  screen_init();
  screen_clear();

  kernel_assert(false);

  // multiboot_info_t* mbi = (multiboot_info_t*)addr;

  screen_write_at('O', SCREEN_COLOR_RED, SCREEN_COLOR_BLACK, 0, 0);
  screen_write_at('K', SCREEN_COLOR_RED, SCREEN_COLOR_BLACK, 0, 1);
  screen_write_at('A', SCREEN_COLOR_RED, SCREEN_COLOR_BLACK, 0, 2);
  screen_write_at('Y', SCREEN_COLOR_RED, SCREEN_COLOR_BLACK, 0, 3);

  while (1) {
    // This allows the CPU to enter a sleep state in which it consumes much
    // less energy. See: https://en.wikipedia.org/wiki/HLT_(x86_instruction)
    __asm__("hlt");
  }
}

