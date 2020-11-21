#ifndef ___MM_PRIVATE
#define ___MM_PRIVATE

#include "mm.h"

void mm_page_early_init(uptr_t kernel_start, uptr_t kernel_end, 
    uptr_t phy_start, uptr_t phy_end);

void mm_frame_early_init(const byte_t *mmap_info, usz_t mmap_info_len);
bo_t mm_frame_get(byte_t **out_frame) base_must_check;

bo_t padd_range_valid(uptr_t start, uptr_t end);
uptr_t padd_start(void);
uptr_t padd_end(void);

#endif
