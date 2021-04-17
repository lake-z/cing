#ifndef ___MM_PRIVATE
#define ___MM_PRIVATE

#include "mm.h"

#define PAGE_SIZE_VALUE_LOG_4K 12
#define PAGE_SIZE_VALUE_4K 4096
typedef enum {
  PAGE_SIZE_4K = PAGE_SIZE_VALUE_4K,
  PAGE_SIZE_2M = 2 * 1024 * 1024,
  PAGE_SIZE_1G = 1024 * 1024 * 1024,
} page_size_t;

/* Virtual address layout of kernel
 +---------------------------+-----------------------+
 |CONSTANT                   |OFFSET                 |
 +---------------------------+-----------------------+
 |VA_48_LOW_START            |0                      |
 +---------------------------+-----------------------+
 |VA_48_LOW_END              |0x00007FFFFFFFFFFF     |
 +---------------------------+-----------------------+
 |VA_48_HIGH_START           |0xFFFF800000000000     |
 |(VA_48_STACK_BP_TOP_GUARD) |                       |
 +---------------------------+-----------------------+
 |VA_48_STACK_BP_TOP         |+1 page                |
 +---------------------------+-----------------------+
 |VA_48_STACK_BP_BOTTOM      |+253 pages             |
 +---------------------------+-----------------------+
 |VA_48_STACK_BP_BOTTOM_GUARD|+1 page                |
 +---------------------------+-----------------------+
 |VA_48_FRAME_BUFFER         |+1 page                |
 +---------------------------+-----------------------+
 |VA_48_PCIE_CFG_START       |High start + 4096 pages|
 +---------------------------+-----------------------+
 |VA_48_DIRECT_ACCESS_PAGE   |+1M pages              |
 |(VA_48_PCIE_CFG_END)       |                       |
 +---------------------------+-----------------------+
 |VA_48_HEAP                 |+1 page                |
 +---------------------------+-----------------------+
 |VA_48_HIGH_END             |0xFFFFFFFFFFFFFFFF     |
 +---------------------------+-----------------------+
 */
#define VA_48_LOW_START u64_literal(0)
#define VA_48_LOW_END u64_literal(0x00007FFFFFFFFFFF)
#define VA_48_HIGH_START u64_literal(0xFFFF800000000000)
#define VA_48_STACK_TOP_GUARD VA_48_HIGH_START
#define VA_48_STACK_TOP (VA_48_HIGH_START + PAGE_SIZE_VALUE_4K)
/* x86 stack grows downward, so this is the page start of the stack bottom. */
#define VA_48_STACK_BOTTOM (VA_48_STACK_TOP + 253 * PAGE_SIZE_VALUE_4K)
#define VA_48_STACK_BOTTOM_GUARD (VA_48_STACK_BOTTOM + PAGE_SIZE_VALUE_4K)
#define VA_48_FRAME_BUFFER (VA_48_STACK_BOTTOM_GUARD + PAGE_SIZE_VALUE_4K)
#define VA_48_PCIE_CFG_START (VA_48_HIGH_START + 4096 * PAGE_SIZE_VALUE_4K)
#define VA_48_PCIE_CFG_END                                                     \
  (VA_48_PCIE_CFG_START + u64_literal(1024) * 1024 * PAGE_SIZE_VALUE_4K)
#define VA_48_DIRECT_ACCESS_PAGE VA_48_PCIE_CFG_END
#define VA_48_HEAP (VA_48_DIRECT_ACCESS_PAGE + PAGE_SIZE_VALUE_4K)
#define VA_48_HIGH_END u64_literal(0xFFFFFFFFFFFFFFFF)

bo_t vadd_get_padd(vptr_t va, uptr_t *out_pa);
bo_t mm_pa_range_valid(uptr_t start, uptr_t end);
uptr_t mm_pa_start(void);
uptr_t mm_pa_end(void);

void mm_frame_early_bootstrap(const byte_t *mmap_info, usz_t mmap_info_len);
void mm_frame_bootstrap(void);

/* Allocate a free frame in early stage.
 *
 * @Returns: The physical address of free frame, which is the same of virtual
 *           address at this early stage. */
bo_t mm_frame_alloc(uptr_t *out_frame) base_must_check;

/* Allocate a free frame
 *
 * @Returns: The physical address of free frame
 */
bo_t mm_frame_alloc_early(byte_t **out_frame) base_must_check;

/* Free a frame. The frame to be freed is pointed by a pointer(virtual address),
 * mm system will gurantee to free page after frame, so the argument will be
 * accessable by pointer. */
void mm_frame_free(
    byte_t *frame_va, /* Virtual address of the frame to be freed */
    uptr_t frame_pa   /* Physical address of the frame to be freed */
);

ucnt_t mm_frame_free_count(void);

/* Whole physical address space will be mapped directly in early bootstrap 
 * stage. */
void mm_page_early_bootstrap(uptr_t kernel_start, uptr_t kernel_end);

/* Bootstrap memory pageing mechanism. */
void mm_page_bootstrap(uptr_t kernel_start,
    uptr_t kernel_end,
    uptr_t boot_stack_bottom,
    uptr_t boot_stack_top);

/* Set up a direct accees for given physical address
 *
 * @Returns: The virtual address (a pointer) to access the physical address. */
vptr_t mm_page_direct_access_setup(uptr_t padd);
void mm_page_direct_access_reset(void);

void mm_heap_bootstrap(void);
vptr_t mm_heap_alloc(usz_t len, usz_t *all_len);
vptr_t mm_heap_alloc_minimum(usz_t *all_len);
void mm_heap_free(vptr_t block_user);

void mm_allocator_bootstrap(void);

#ifdef BUILD_SELF_TEST_ENABLED
/* Built-in tests declarations */
void test_heap(void);
void test_allocator(void);
#endif

#endif
