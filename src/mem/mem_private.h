/* Private header of memory management subsystem. */
#ifndef ___MEM_PRIVATE
#define ___MEM_PRIVATE

#include "mem.h"

#define PAGE_SIZE_VALUE_4K 4096

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
#define VA_48_GRIP_PAGE VA_48_PCIE_CFG_END
#define VA_48_HEAP (VA_48_GRIP_PAGE + PAGE_SIZE_VALUE_4K)
#define VA_48_HIGH_END u64_literal(0xFFFFFFFFFFFFFFFF)

typedef enum {
  MEM_BOOTSTRAP_STAGE_0 = 0,
  MEM_BOOTSTRAP_STAGE_1 = 1,
  MEM_BOOTSTRAP_STAGE_2 = 2,
} mem_bootstrap_stage_t;

/* Memory management subsystem modules shared bootstrap stage. */
extern mem_bootstrap_stage_t boot_stage;

void mem_frame_bootstrap_1(const byte_t *mb_elf,
    usz_t mb_elf_len,
    const byte_t *mb_mmap,
    usz_t mb_mmap_len);

base_must_check bo_t mem_frame_alloc_bootstrap(byte_t **out_frame);

uptr_t mem_pa_start(void);
uptr_t mem_pa_end(void);
bo_t mem_pa_range_valid(uptr_t start, uptr_t end);

void mem_page_bootstrap_1(void);

uptr_t mem_pa_ker_start(void);
uptr_t mem_pa_ker_end(void);

#endif
