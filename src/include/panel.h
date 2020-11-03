#ifndef ___PANEL
#define ___PANEL

#include "base.h"
typedef struct panel_text panel_text_t;
void panel_start(void);
panel_text_t *panel_get_log_text(void);

usz_t panel_text_width(panel_text_t *txt);
void panel_text_draw(panel_text_t *txt);
bo_t panel_text_write_row(panel_text_t *txt, const ch_t *str, usz_t str_len);

#endif
