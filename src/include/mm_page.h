#ifndef ___MM_PAGE
#define ___MM_PAGE

#include "base.h"

typedef enum {
  PAGE_SIZE_4K = 4 * 1024,
  PAGE_SIZE_2M = 2 * 1024 * 1024,
  PAGE_SIZE_1G = 1024 * 1024 * 1024,
} page_size_t;

typedef u64_t page_no_t;
typedef u8_t page_tab_level_t;

void page_load_early_tab(uptr_t kernel_start, uptr_t kernel_end);

#endif
