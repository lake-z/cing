#ifndef ___MM_PRIVATE
#define ___MM_PRIVATE

#include "mm.h"
#include "mm_page.h"

void mm_frame_init(const byte_t *mmap_info, usz_t mmap_info_len);
bo_t mm_phy_addr_range_valid(uptr_t start, uptr_t end);

void mm_page_init_mmap_info(const byte_t *ptr, usz_t size);
void page_early_tab_load(uptr_t kernel_start, uptr_t kernel_end);

#endif
