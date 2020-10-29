#include "kernel_panic.h"
#include "drivers_screen.h"
#include <stdlib.h>

base_private void _unsigned_to_str(usz_t uval, char *buf, usz_t buf_cap)
{
  usz_t buf_off;
  usz_t digit_cnt;
  usz_t uval_origin = uval;

  /* Counting digits in total*/
  digit_cnt = 0;
  do {
    uval /= 10;
    digit_cnt++;
  } while (uval > 0);

  /* No enough space, output 'ER' to denote an error to be noticed. */
  if ((digit_cnt + 1) > buf_cap) {
    if (buf_cap >= 3) {
      buf[0] = 'E';
      buf[1] = 'R';
      buf[2] = '\0';
    } else if (buf_cap == 2) {
      buf[0] = 'E';
      buf[1] = '\0';
    } else if (buf_cap == 1) {
      /* buf_cap equals 1, only output an char E here to denote an error.
       * NULL terminator can not be included as we think to 
       * discover an error is more important than elegant display. */
      buf[0] = 'E';
    } else {
      /* buf_cap must equals 0, we can do nothing overwriting no memory */
    }
    return;
  }

  uval = uval_origin;
  buf_off = digit_cnt - 1;
  while (true) {
    usz_t digit;

    digit = uval % 10;
    buf[buf_off] = (ch_t)(digit + '0');

    buf_off--;
    uval /= 10;
    if (uval == 0) {
      break;
    }
  }

  buf[digit_cnt] = '\0';
}

base_no_return _kernel_panic(const char *file, usz_t line, const char *msg)
{
  char ln_str[128];
  screen_write_str("Kernel panic!", SCREEN_COLOR_WHITE, SCREEN_COLOR_RED,
                   SCREEN_HEIGHT - 5, 0);
  screen_write_str(msg, SCREEN_COLOR_WHITE, SCREEN_COLOR_RED, SCREEN_HEIGHT - 4,
                   0);
  screen_write_str("File:", SCREEN_COLOR_WHITE, SCREEN_COLOR_RED,
                   SCREEN_HEIGHT - 3, 0);
  screen_write_str(file, SCREEN_COLOR_WHITE, SCREEN_COLOR_RED,
                   SCREEN_HEIGHT - 2, 0);
  _unsigned_to_str(line, ln_str, 128);
  screen_write_str(ln_str, SCREEN_COLOR_WHITE, SCREEN_COLOR_RED,
                   SCREEN_HEIGHT - 1, 0);

  while (1) {
    __asm__("hlt");
  }
}

base_no_return
_kernel_assert_fail(const char *expr, const char *file, usz_t line)
{
  char ln_str[128];
  screen_write_str("Kernel assertion failure!", SCREEN_COLOR_WHITE,
                   SCREEN_COLOR_RED, SCREEN_HEIGHT - 7, 0);
  screen_write_str("Expression:", SCREEN_COLOR_WHITE, SCREEN_COLOR_RED,
                   SCREEN_HEIGHT - 6, 0);
  screen_write_str(expr, SCREEN_COLOR_WHITE, SCREEN_COLOR_RED,
                   SCREEN_HEIGHT - 5, 0);
  screen_write_str("File:", SCREEN_COLOR_WHITE, SCREEN_COLOR_RED,
                   SCREEN_HEIGHT - 4, 0);
  screen_write_str(file, SCREEN_COLOR_WHITE, SCREEN_COLOR_RED,
                   SCREEN_HEIGHT - 3, 0);

  _unsigned_to_str(line, ln_str, 128);
  screen_write_str("Line:", SCREEN_COLOR_WHITE, SCREEN_COLOR_RED,
                   SCREEN_HEIGHT - 2, 0);
  screen_write_str(ln_str, SCREEN_COLOR_WHITE, SCREEN_COLOR_RED,
                   SCREEN_HEIGHT - 1, 0);

  while (1) {
    __asm__("hlt");
  }
}

uintptr_t __stack_chk_guard = 0x595e9fbd94fda766;

/**
 * Basic stack smashing protector implementation
 * Based on https://wiki.osdev.org/Stack_Smashing_Protector
 */
void __stack_chk_fail(void)
{
  kernel_panic("Stack smashing detected");
}
