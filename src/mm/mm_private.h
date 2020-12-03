#ifndef ___MM_PRIVATE
#define ___MM_PRIVATE

#include "mm.h"

#define PAGE_SIZE_VALUE_4K 4096

/* Virtual address layout of kernel
 * --------
 * [*] Low memory < kernel_start: never used by os
 * [*] kernel_start .. kernel_end is a direct mapping
 * [*] VADD_48_HIGH_START .. VADD_48_HIGH_START + 256 pages, the stack
 * [*] VADD_48_HIGH_START + 257 pages: the temporary direct access page
 * [*] VADD_48_HIGH_START + 258 pages...: the heap
 */
#define VADD_LOW_START u64_literal(0)
#define VADD_48_LOW_END u64_literal(0x00007fffffffffff)
#define VADD_48_HIGH_START u64_literal(0xffff800000000000)
#define VADD_STACK_BP_TOP_GUARD VADD_48_HIGH_START
#define VADD_STACK_BP_TOP (VADD_48_HIGH_START + 1 * PAGE_SIZE_VALUE_4K)
#define VADD_STACK_BP_BOTTOM (VADD_48_HIGH_START + 254 * PAGE_SIZE_VALUE_4K)
#define VADD_STACK_BP_BOTTOM_GUARD                                             \
  (VADD_48_HIGH_START + 255 * PAGE_SIZE_VALUE_4K)
#define VADD_DIRECT_ACCESS_PAGE (VADD_48_HIGH_START + 256 * PAGE_SIZE_VALUE_4K)
#define VADD_HEAP_START (VADD_48_HIGH_START + 257 * PAGE_SIZE_VALUE_4K)
#define VADD_HIGH_END u64_literal(0xffffffffffffffff)

bo_t vadd_get_padd(vptr_t va, uptr_t *out_pa);
bo_t padd_range_valid(uptr_t start, uptr_t end);
uptr_t padd_start(void);
uptr_t padd_end(void);

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

void mm_page_early_init(uptr_t kernel_start, uptr_t kernel_end);
void mm_page_init(uptr_t kernel_start,
    uptr_t kernel_end,
    uptr_t boot_stack_bottom,
    uptr_t boot_stack_top);

/* Set up a direct accees for given physical address
 *
 * @Returns: The virtual address (a pointer) to access the physical address. */
vptr_t mm_page_direct_access_setup(uptr_t padd);
void mm_page_direct_access_reset(void);
bo_t mm_page_map(uptr_t va, uptr_t pa);

void mm_heap_init(void);
byte_t *mm_heap_alloc(usz_t len, usz_t *all_len);

#endif
