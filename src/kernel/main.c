#include <stdint.h>
#include <sys/types.h>
#include <stddef.h>
#include <string.h>

/// The address of the frame buffer.
#define VIDEO_ADDRESS 0xB8000
#define SCREEN_WIDTH  80
#define SCREEN_HEIGHT 25

#define FB_COMMAND_PORT      0x3D4
#define FB_DATA_PORT         0x3D5
#define FB_HIGH_BYTE_COMMAND 14
#define FB_LOW_BYTE_COMMAND  15

static const uint8_t COLOR_BLACK = 0;
static const uint8_t COLOR_BLUE = 1;
static const uint8_t COLOR_GREEN = 2;
static const uint8_t COLOR_CYAN = 3;
static const uint8_t COLOR_RED = 4;
static const uint8_t COLOR_MAGENTA = 5;
static const uint8_t COLOR_BROWN = 6;
static const uint8_t COLOR_LIGHT_GREY = 7;
static const uint8_t COLOR_DARK_GREY = 8;
static const uint8_t COLOR_LIGHT_BLUE = 9;
static const uint8_t COLOR_LIGHT_GREEN = 10;
static const uint8_t COLOR_LIGHT_CYAN = 11;
static const uint8_t COLOR_LIGHT_RED = 12;
static const uint8_t COLOR_LIGHT_MAGENTA = 13;
static const uint8_t COLOR_LIGHT_BROWN = 14;
static const uint8_t COLOR_WHITE = 15;

uint8_t screen_scheme;
char* framebuffer;
int screen_col;
int screen_row;

uint8_t color_scheme(uint8_t fg, uint8_t bg)
{
  return fg | bg << 4;
}

void screen_color_scheme(uint8_t fg, uint8_t bg)
{
  screen_scheme = color_scheme(fg, bg);
}

void screen_init()
{
  framebuffer = (char*)VIDEO_ADDRESS;
  screen_col = 0;
  screen_row = 0;
  screen_color_scheme(COLOR_WHITE, COLOR_BLACK);
}

void screen_write_at(char c, uint8_t scheme, int x, int y)
{
  int offset = 2 * (y * SCREEN_WIDTH + x);

  framebuffer[offset] = c;
  framebuffer[offset + 1] = scheme;
}

void screen_clear()
{
  // memset((char*)VIDEO_ADDRESS, 0, 2 * SCREEN_HEIGHT * SCREEN_WIDTH);
  char *ptr = (char *)VIDEO_ADDRESS;
  for (size_t i = 0; i < 2 * SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
    ptr[i] = '\0';
  }  

  screen_col = 0;
  screen_row = 0;
}

void kmain(uint64_t addr)
{
  screen_init();
  screen_clear();
  uint8_t colors = color_scheme(COLOR_RED, COLOR_BLACK);

  // multiboot_info_t* mbi = (multiboot_info_t*)addr;

  screen_write_at('O', colors, 10, 20);
  screen_write_at('K', colors, 11, 20);
  screen_write_at('A', colors, 12, 20);
  screen_write_at('Y', colors, 13, 20);

  while (1) {
    // This allows the CPU to enter a sleep state in which it consumes much
    // less energy. See: https://en.wikipedia.org/wiki/HLT_(x86_instruction)
    __asm__("hlt");
  }
}

