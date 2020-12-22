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
  vptr_t ret;
  area_t *area;
  uptr_t ptr;

  area = all->list;
  if (area == NULL) {
    need = size + align; /* Gurantees to satisfy an aligned allocate */
    area = _allocator_new_area(all, need);
    kernel_assert_d(all->list == area);
  }

  ptr = _area_free_start(area);
  slop = ptr % align;
  if (slop > 0) {
    slop = align - slop;
  }
  need = size + slop;

  if (_area_free_len(area) < need) {
    area = _allocator_new_area(all, need);
    kernel_assert_d(all->list == area);
  }
  kernel_assert_d(_area_free_len(area) >= need);

  ret = (vptr_t)(_area_free_start(area) + slop);
  area->use_len += need;

  kernel_assert_d(area->use_len < area->block_len);
  return ret;
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
