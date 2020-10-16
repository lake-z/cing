#include "containers_string.h"
#include "drivers_screen.h"
#include "kernal_main.h"
#include "kernel_panic.h"
#include <string.h> // TODO: Remove this later

#define FB_COMMAND_PORT      0x3D4
#define FB_DATA_PORT         0x3D5
#define FB_HIGH_BYTE_COMMAND 14
#define FB_LOW_BYTE_COMMAND  15

base_private void process_boot_info(uch_t *addr)
{
  base_private const usz_t _MSG_LEN = 128;
  ch_t msg[_MSG_LEN];
  usz_t msg_ptr;
  ch_t *str;
  usz_t row_no;

  row_no = 0;

  msg_ptr = 0;
  msg_ptr += str_buf_marshal_uint(msg, msg_ptr, _MSG_LEN, row_no);
  str = (ch_t *)"BOOT INFO: ";
  msg_ptr += str_buf_marshal_str(msg, msg_ptr, _MSG_LEN, str, str_len(str));
  msg_ptr += str_buf_marshal_uint(msg, msg_ptr, _MSG_LEN, (u64_t)addr[0]);
  msg[msg_ptr] = '\0';
  screen_write_str(msg, SCREEN_COLOR_LIGHT_GREY, SCREEN_COLOR_BLACK, 
    row_no++, 0);

  msg_ptr = 0;
  msg_ptr += str_buf_marshal_uint(msg, msg_ptr, _MSG_LEN, row_no);
  str = (ch_t *)"flags: ";
  msg_ptr += str_buf_marshal_str(msg, msg_ptr, _MSG_LEN, str, str_len(str));
  if(byte_is_bit_set(addr[0], 0)) {
    str = (ch_t *)"mem* enabled.";
  } else {
    str = (ch_t *)"mem* disabled.";
  }
  msg_ptr += str_buf_marshal_str(msg, msg_ptr, _MSG_LEN, str, str_len(str));
  msg[msg_ptr] = '\0';
  screen_write_str(msg, SCREEN_COLOR_BLUE, SCREEN_COLOR_BLACK, row_no++, 0);

  msg_ptr = 0;
  msg_ptr += str_buf_marshal_uint(msg, msg_ptr, _MSG_LEN, row_no);
  str = (ch_t *)"flags: ";
  msg_ptr += str_buf_marshal_str(msg, msg_ptr, _MSG_LEN, str, str_len(str));
  if(byte_is_bit_set(addr[0], 1)) {
    str = (ch_t *)"boot_device enabled.";
  } else {
    str = (ch_t *)"boot_device disabled.";
  }
  msg_ptr += str_buf_marshal_str(msg, msg_ptr, _MSG_LEN, str, str_len(str));
  msg[msg_ptr] = '\0';
  screen_write_str(msg, SCREEN_COLOR_BLUE, SCREEN_COLOR_BLACK, row_no++, 0);

  msg_ptr = 0;
  msg_ptr += str_buf_marshal_uint(msg, msg_ptr, _MSG_LEN, row_no);
  str = (ch_t *)"flags: ";
  msg_ptr += str_buf_marshal_str(msg, msg_ptr, _MSG_LEN, str, str_len(str));
  if(byte_is_bit_set(addr[0], 2)) {
    str = (ch_t *)"cmdline enabled.";
  } else {
    str = (ch_t *)"cmdline disabled.";
  }
  msg_ptr += str_buf_marshal_str(msg, msg_ptr, _MSG_LEN, str, str_len(str));
  msg[msg_ptr] = '\0';
  screen_write_str(msg, SCREEN_COLOR_BLUE, SCREEN_COLOR_BLACK, row_no++, 0);
  
}

void kernal_main(u64_t addr)
{
  screen_init();
  screen_clear();

  // multiboot_info_t* mbi = (multiboot_info_t*)addr;
  process_boot_info((uch_t *)addr);

  while (1) {
    // This allows the CPU to enter a sleep state in which it consumes much
    // less energy. See: https://en.wikipedia.org/wiki/HLT_(x86_instruction)
    __asm__("hlt");
  }
}

