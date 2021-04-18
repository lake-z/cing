/* Private header of memory management subsystem. */
#ifndef ___MEM_PRIVATE
#define ___MEM_PRIVATE

#include "mem.h"

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
 |VA_48_GRIP_PAGE            |+1M pages              |
 |(VA_48_PCIE_CFG_END)       |                       |
 +---------------------------+-----------------------+
 |VA_48_HEAP                 |+1 pages               |
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
#define VA_48_FRAME_BUFFER_END (VA_48_HIGH_START + 4096 * PAGE_SIZE_VALUE_4K)
#define VA_48_PCIE_CFG_START VA_48_FRAME_BUFFER_END
#define VA_48_PCIE_CFG_END                                                     \
  (VA_48_PCIE_CFG_START + u64_literal(1024) * 1024 * PAGE_SIZE_VALUE_4K)
#define VA_48_GRIP_PAGE VA_48_PCIE_CFG_END
#define VA_48_HEAP (VA_48_GRIP_PAGE + PAGE_SIZE_VALUE_4K)
#define VA_48_HIGH_END u64_literal(0xFFFFFFFFFFFFFFFF)

typedef enum {
  MEM_BOOTSTRAP_STAGE_0 = 0,
  MEM_BOOTSTRAP_STAGE_1 = 1,
  MEM_BOOTSTRAP_STAGE_2 = 2,
  MEM_BOOTSTRAP_STAGE_FINISH = 99,
} mem_bootstrap_stage_t;

typedef struct mem_heap mem_heap_t;

void mem_frame_bootstrap_1(const byte_t *mb_elf,
    usz_t mb_elf_len,
    const byte_t *mb_mmap,
    usz_t mb_mmap_len);
void mem_page_bootstrap_1(void);
void mem_page_bootstrap_2(void);
void mem_frame_bootstrap_2(void);

base_must_check bo_t mem_frame_alloc(byte_t **out_frame);

/* Initialize memory heap on a preallocated memroy area. 
 * @return true for succ, or false for failure. */
//bo_t mem_heap_init_pre(
//  byte_t *buf, /* Memory area to initialize heap. */
//  usz_t buf_len, /* Total length of memory area. */
//  usz_t map_size, /* Map size for heap management. */
//  uptr_t heap_add, /* Heap start size, must be aligned to @PAGE_SIZE_4K */
//  usz_t heap_size, /* Heap size, must be a interal multiple of @PAGE_SIZE_4K */
//  mem_heap_t **out_heap, /* Initialized memory heap. */
//  byte_t **out_next /* End pointer for used memory */
//);

base_must_check mem_heap_t *mem_heap_new_bootstrap(
    uptr_t heap_add, usz_t heap_size);

uptr_t mem_pa_start(void);
uptr_t mem_pa_end(void);
bo_t mem_pa_range_valid(uptr_t start, uptr_t end);
bo_t pa_range_overlaps(uptr_t a1, usz_t len1, uptr_t a2, usz_t len2);
uptr_t mem_pa_ker_start(void);
uptr_t mem_pa_ker_end(void);

pa_list_t *pa_list_new_bootstrap(u64_t n_range);
void pa_list_free_bootstrap(pa_list_t *list);
void pa_list_set_range(pa_list_t *list, u64_t range, uptr_t pa, u64_t n_pg);
ucnt_t pa_list_n_page(pa_list_t *list);
u64_t pa_list_range_n_page(pa_list_t *list, u64_t range_idx);
uptr_t pa_list_range_pa(pa_list_t *list, u64_t range_idx);
u64_t pa_list_n_range(pa_list_t *list);

/* Memory management subsystem modules shared bootstrap stage. */
extern mem_bootstrap_stage_t boot_stage;

#endif
