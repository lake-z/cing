#ifndef ___DRIVERS_VESA
#define ___DRIVERS_VESA

#include "base.h"
#include "drivers_vesa.h"

void d_vesa_bootstrap(
    const byte_t *fb, /* Frame buffer tag from multi boot info. */
    usz_t fb_len /* Length */
);

byte_t *d_vesa_get_frame_buffer(void);

usz_t d_vesa_get_frame_buffer_len(void);

void d_vesa_draw_pixel(u16_t x, u16_t y, u8_t red, u8_t green, u8_t blue);

#endif