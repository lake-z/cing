/* Video Electronics Standards Association video driver. */

#include "drivers_vesa.h"
#include "kernel_panic.h"
#include "log.h"

/* Frame buffer address, initialized at legacy VGA address. */
base_private byte_t *_buf = (byte_t *)0xB8000;
base_private usz_t _width;
base_private usz_t _height;
base_private usz_t _depth;

typedef struct frame_buffer_info {
  u64_t paddr;
  u32_t pitch;
  u32_t width;
  u32_t height;
  u8_t depth;
  u8_t colour_type;
  u8_t reserved;
  u8_t red_offset;
  u8_t red_size;
  u8_t green_offset;
  u8_t green_size;
  u8_t blue_offset;
  u8_t blue_size;
} base_struct_packed frame_buffer_info_t;

void d_vesa_draw_pixel(usz_t x, usz_t y, u8_t red, u8_t green, u8_t blue)
{
  usz_t idx;
  kernel_assert(x < _width);
  kernel_assert(y < _height);

  idx = (y * _width + x) * _depth;
  _buf[idx] = blue;
  _buf[idx + 1] = green;
  _buf[idx + 2] = red;
}

byte_t *d_vesa_get_frame_buffer(void)
{
  return _buf;
}

void d_vesa_set_frame_buffer(byte_t *fb)
{
  _buf = fb;
}

usz_t d_vesa_get_frame_buffer_len(void)
{
  return _width * _height * _depth;
}

usz_t d_vesa_get_width(void)
{
  return _width;
}

usz_t d_vesa_get_height(void)
{
  return _height;
}

void d_vesa_bootstrap(const byte_t *fb, usz_t fb_len)
{
  frame_buffer_info_t *info = (frame_buffer_info_t *)fb;

  kernel_assert(info->colour_type == 1); /* Suports direct RGB only. */
  kernel_assert(info->depth == 32);      /* Supports 32-bit true colour only. */
  kernel_assert(info->reserved == 0);
  kernel_assert(fb_len == 30);

  _buf = (byte_t *)info->paddr;
  _width = info->width;
  _height = info->height;
  _depth = info->depth;
  kernel_assert(_depth % 8 == 0);
  _depth = _depth / 8;

  log_line_format(LOG_LEVEL_INFO, "Frame buffer: %lux%lu, %lu, pa: %lu", _width,
      _height, _depth, (uptr_t)_buf);
}
