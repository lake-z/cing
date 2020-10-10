#include <stdlib.h>
#include "drivers_screen.h"
#include "kernel_panic.h"

base_no_return _kernel_panic(
  const char *msg
)
{
  screen_write_str("Kernel panic!", SCREEN_COLOR_WHITE, SCREEN_COLOR_RED, 
    SCREEN_HEIGHT - 2, 0);
  screen_write_str(msg, SCREEN_COLOR_WHITE, SCREEN_COLOR_RED, 
    SCREEN_HEIGHT - 1, 0);

  while (1) {
    // This allows the CPU to enter a sleep state in which it consumes much
    // less energy. See: https://en.wikipedia.org/wiki/HLT_(x86_instruction)
    __asm__("hlt");
  }
}

base_no_return _kernel_assert_fail(const char *expr, const char *file, usz_t line)
{
  usz_t row = 0;
  char ln_str[128];
  screen_write_str("Kernel assertion failure!", SCREEN_COLOR_WHITE, SCREEN_COLOR_RED, 
    row++, 0);
  screen_write_str("Expression:", SCREEN_COLOR_WHITE, SCREEN_COLOR_RED, 
    row++, 0);
  screen_write_str(expr, SCREEN_COLOR_WHITE, SCREEN_COLOR_RED, row++, 0);
  screen_write_str("File:", SCREEN_COLOR_WHITE, SCREEN_COLOR_RED, row++, 0);
  screen_write_str(file, SCREEN_COLOR_WHITE, SCREEN_COLOR_RED, row++, 0);

  /*
  itoa(line, ln_str, 10);
  screen_write_str("Line no.:", SCREEN_COLOR_WHITE, SCREEN_COLOR_RED, row++, 0);
  screen_write_str(ln_str, SCREEN_COLOR_WHITE, SCREEN_COLOR_RED, row++, 0);
  */

  while (1) {
    // This allows the CPU to enter a sleep state in which it consumes much
    // less energy. See: https://en.wikipedia.org/wiki/HLT_(x86_instruction)
    __asm__("hlt");
  }
}
