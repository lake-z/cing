#ifndef ___MM_PRIVATE
#define ___MM_PRIVATE

#include "mm.h"

void mm_page_init_mmap_info(const byte_t *ptr, usz_t size);
bo_t mm_page_phy_addr_range_valid(uptr_t start, uptr_t end);

#endif
