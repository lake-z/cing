#ifndef ___VIDEO
#define ___VIDEO

#include "base.h"

usz_t video_char_row_max(void);
usz_t video_char_col_max(void);

void video_draw_char(usz_t row, usz_t col, u8_t ch);

void test_video(void);

#endif
