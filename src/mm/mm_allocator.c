#include "kernel_panic.h"
#include "log.h"
#include "mm_private.h"
#include "util.h"

#define _NODE_ALLOCATOR_CAP 64

typedef struct area area_t;
struct area {
  byte_t *block;
  usz_t block_len;
  usz_t use_len;
  area_t *prev_area;
};

struct mm_allocator {
  area_t *list;
  mm_allocator_t *next_free;
};

base_private mm_allocator_t *_free_alls;

void mm_allocator_bootstrap(void)
{
  _free_alls = NULL;
}

base_private usz_t _area_free_len(area_t *a)
{
  return a->block_len - a->use_len;
}

base_private uptr_t _area_free_start(area_t *a)
{
  kernel_assert_d(_area_free_len(a) > 0);
  return (uptr_t)a->block + a->use_len;
}

base_private uptr_t _area_block_end(area_t *a)
{
  return (uptr_t)(a->block) + a->block_len;
}

base_private void _allocator_init(mm_allocator_t *all)
{
  all->list = NULL;
}

mm_allocator_t *mm_allocator_new(void)
{
  mm_allocator_t *all = _free_alls;

  if (all == NULL) {
    usz_t heap_len;
    uptr_t heap = (uptr_t)mm_heap_alloc_minimum(&heap_len);
    all = (mm_allocator_t *)mm_align_up(heap, sizeof(mm_allocator_t *));
    kernel_assert_d(((uptr_t)all + sizeof(mm_allocator_t) - heap) <= heap_len);

    /* Divide allocate memory block into a lot of allocator structures to be 
     * used later. */
    while (((uptr_t)all + sizeof(mm_allocator_t) - heap) <= heap_len) {
      all->next_free = _free_alls;
      _free_alls = all;
      all += sizeof(mm_allocator_t);
      all =
          (mm_allocator_t *)mm_align_up((uptr_t)all, sizeof(mm_allocator_t *));
    }
    all = _free_alls;
  }

  kernel_assert_d(all != NULL);

  _free_alls = all->next_free;
  all->next_free = NULL;

  _allocator_init(all);

  return all;
}

base_private area_t *_allocator_new_area(mm_allocator_t *all, usz_t block_min)
{
  area_t *area;
  usz_t block_size;
  usz_t need_size;
  mm_allocator_t **guard;

  need_size = block_min + sizeof(area_t) + sizeof(mm_allocator_t *);

  area = mm_heap_alloc(need_size, &block_size);
  area->block = (byte_t *)((uptr_t)area + sizeof(area_t));

  guard =
      (mm_allocator_t **)((uptr_t)area + block_size - sizeof(mm_allocator_t *));
  kernel_assert(mm_align_check((uptr_t)guard, sizeof(mm_allocator_t *)));
  *guard = all;

  area->block_len = (uptr_t)guard - (uptr_t)(area->block);
  kernel_assert(area->block_len >= block_min);

  area->use_len = 0;
  area->prev_area = all->list;
  all->list = area;

  return area;
}

vptr_t mm_allocate(mm_allocator_t *all, usz_t size, usz_t align)
{
  usz_t slop;
  usz_t need;
  uptr_t ret;
  area_t *area;

  area = all->list;
  if (area == NULL) {
    need = size + align; /* Gurantees to satisfy an aligned allocate */
    area = _allocator_new_area(all, need);
    kernel_assert_d(all->list == area);
  }

  ret = _area_free_start(area);
  slop = ret % align;
  if (slop > 0) {
    slop = align - slop;
  }
  need = size + slop;

  if (_area_free_len(area) < need) {
    need = size + align; /* Gurantees to satisfy an aligned allocate */
    area = _allocator_new_area(all, need);
    kernel_assert_d(all->list == area);
    kernel_assert_d(_area_free_len(area) >= need);
  }

  ret = _area_free_start(area);
  slop = ret % align;
  if (slop > 0) {
    slop = align - slop;
  }
  need = size + slop;
  kernel_assert_d(_area_free_len(area) >= need);
  ret += slop;
  area->use_len += need;

  kernel_assert_d(area->use_len < area->block_len);
  kernel_assert_d(mm_align_check(ret, align));
  return (vptr_t)ret;
}

void mm_allocator_free(mm_allocator_t *all)
{
  area_t *a = all->list;
  while (a != NULL) {
    vptr_t free_ptr;
    mm_allocator_t **guard = (mm_allocator_t **)_area_block_end(a);
    kernel_assert_d(mm_align_check((uptr_t)guard, sizeof(mm_allocator_t *)));
    kernel_assert_d((*guard) == all);

    free_ptr = a;
    a = a->prev_area;
    mm_heap_free(free_ptr);
  }

  all->list = NULL;
  all->next_free = _free_alls;
  _free_alls = all;
}

#ifdef BUILD_SELF_TEST_ENABLED
/* Built-in tests */

#define _TEST_MAX_UNITS 32
#define _TEST_UNIT_MEMS_CAP 128
#define _TEST_MAX_ALLOC 1048576 /* 1MB */
#define _TEST_MAX_ALIGN 1048576 /* 1MB */

typedef struct test_allocator_unit {
  mm_allocator_t *all;
  byte_t *mems[_TEST_UNIT_MEMS_CAP];
  usz_t mem_lens[_TEST_UNIT_MEMS_CAP];
  usz_t mem_cnt;
} test_allocator_unit_t;

base_private bo_t _test_unit_free(test_allocator_unit_t *unit)
{
  byte_t *mem;
  usz_t len;
  byte_t by;

  for (usz_t i = 0; i < unit->mem_cnt; i++) {
    mem = unit->mems[i];
    len = unit->mem_lens[i];
    by = (byte_t)(i % BYTE_MAX);
    for (usz_t by_idx; by_idx < len; by_idx++) {
      kernel_assert(mem[by_idx] == by);
    }
  }

  mm_allocator_free(unit->all);
  return true;
}

base_private bo_t _test_rand_alloc(test_allocator_unit_t *unit)
{
  if ((unit->mem_cnt) < _TEST_UNIT_MEMS_CAP) {
    usz_t size;
    usz_t align;
    byte_t *mem;
    byte_t by;

    if (unit->mem_cnt > 0) {
      size = util_rand_int_next(unit->mem_lens[unit->mem_cnt - 1]);
    } else {
      size = util_rand_int_next(_TEST_MAX_ALLOC);
    }
    align = util_rand_int_next(size);
    align = align % _TEST_MAX_ALIGN + 1;
    size = size % _TEST_MAX_ALLOC + 1;

    mem = (byte_t *)mm_allocate(unit->all, size, align);
    kernel_assert(mm_align_check((uptr_t)mem, align));
    unit->mems[unit->mem_cnt] = mem;
    unit->mem_lens[unit->mem_cnt] = size;
    by = (byte_t)(unit->mem_cnt % BYTE_MAX);
    mm_fill_bytes(mem, size, by);
    (unit->mem_cnt) += 1;

    return true;
  } else {
    return false;
  }
}

void test_allocator(void)
{
  usz_t op_cnt = 10000;
  u64_t op_type = 123;
  test_allocator_unit_t units[_TEST_MAX_UNITS];
  usz_t unit_cnt = 0;
  usz_t unit_idx = 0;
  bo_t ok;
  u64_t op_clock = 0;

  for (usz_t op = 0; op < op_cnt;) {
    op_clock++;
    op_type = util_rand_int_next(op_type + op_clock);
    op_type = op_type % 9;

    switch (op_type) {
    case 0:
      if (unit_cnt < _TEST_MAX_UNITS) {
        units[unit_cnt].all = mm_allocator_new();
        units[unit_cnt].mem_cnt = 0;
        unit_cnt++;
        op++;
      }
      break;
    case 1:
      if (unit_cnt > 0) {
        ok = _test_unit_free(&units[unit_cnt - 1]);
        if (ok) {
          unit_cnt--;
          op++;
        }
      }
      break;
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
      if (unit_cnt > 0) {
        unit_idx = util_rand_int_next(unit_idx);
        unit_idx = unit_idx % unit_cnt;
        ok = _test_rand_alloc(&units[unit_idx]);
        if (ok) {
          op++;
        }
      }
      break;
    default:
      kernel_panic("Impossible");
      break;
    }
  }
  log_builtin_test_pass();
}
#endif
