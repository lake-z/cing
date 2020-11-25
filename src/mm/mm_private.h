#ifndef ___MM_PRIVATE
#define ___MM_PRIVATE

#include "mm.h"

void mm_page_early_init(uptr_t kernel_start, uptr_t kernel_end);
void mm_page_init(uptr_t kernel_start, uptr_t kernel_end);

/* Set up a direct accees for given physical address
 *
 * @Returns: The virtual address (a pointer) to access the physical address. */
vptr_t mm_page_direct_access_setup(uptr_t padd);
void mm_page_direct_access_reset(void);

void mm_frame_early_init(const byte_t *mmap_info, usz_t mmap_info_len);
void mm_frame_init(void);

/* Allocate a free frame in early stage.
 *
 * @Returns: The physical address of free frame, which is the same of virtual
 *           address at this early stage. */
bo_t mm_frame_get(uptr_t *out_frame) base_must_check;

/* Allocate a free frame
 *
 * @Returns: The physical address of free frame
 */
bo_t mm_frame_get_early(byte_t **out_frame) base_must_check;

/* Free a frame. The frame to be freed is pointed by a pointer(virtual address),
 * mm system will gurantee to free page after frame, so the argument will be
 * accessable by pointer. */
void mm_frame_return(
    byte_t *frame_va, /* Virtual address of the frame to be freed */
    uptr_t frame_pa   /* Physical address of the frame to be freed */
);

u64_t mm_frame_free_count(void);

bo_t vadd_get_padd(vptr_t va, uptr_t *out_pa);

bo_t padd_range_valid(uptr_t start, uptr_t end);
uptr_t padd_start(void);
uptr_t padd_end(void);

#endif
