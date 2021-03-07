#ifndef ___DRIVERS_VESA
#define ___DRIVERS_VESA

#include "base.h"

void d_vesa_bootstrap(
    const byte_t *fb, /* Frame buffer tag from multi boot info. */
    usz_t fb_len      /* Length */
);

byte_t *d_vesa_get_frame_buffer(void);
void d_vesa_set_frame_buffer(byte_t *fb);
usz_t d_vesa_get_frame_buffer_len(void);
usz_t d_vesa_get_width(void);
usz_t d_vesa_get_height(void);

void d_vesa_draw_pixel(usz_t x, usz_t y, u8_t red, u8_t green, u8_t blue);

#endif
