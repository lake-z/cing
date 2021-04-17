#include "kernel_panic.h"
#include "mem_private.h"
#include "util.h"

#define _CLASS_MAX 15
#define _BOOTSTRAP_BUFF_SIZE 65536
base_private const u64_t _MAP_NULL = U64_MAX;

struct mem_heap {
  uptr_t add_0;
  uptr_t add_z;
  u64_t *map;
  usz_t map_size;
};

byte_t _bootstrap_buff[_BOOTSTRAP_BUFF_SIZE] base_align(PAGE_SIZE_VALUE_4K);

base_private u64_t _page_max(mem_heap_t *heap)
{
  return (heap->add_z - heap->add_0) / PAGE_SIZE_4K;
}

base_private usz_t _map_header_len(void)
{
  /* @_CLASS_MAX free lists, one slots list. */
  return (sizeof(u64_t) * 2 * (_CLASS_MAX + 1));
}

/* Caculate max heap size can be managed by given heap map size. */
base_private usz_t _map_max_heap(usz_t map_size)
{
  usz_t size = map_size;

  kernel_assert(map_size % PAGE_SIZE_VALUE_4K == 0);

  /* Map header size. */
  size -= _map_header_len();
  kernel_assert(size % (sizeof(u64_t) * 2) == 0);

  /* If we link all pages one by one on on list, that is the minial max 
   * heap size we can manage. We divide one more 2 here to make the calculate
   * safe. TODO: Improve the effiency. */
  size = size / (sizeof(u64_t) * 2 * 2);

  size *= PAGE_SIZE_VALUE_4K;

  return size;
}

base_private u64_t _map_slot_max(mem_heap_t *heap)
{
  usz_t size = heap->map_size - _map_header_len();
  return size / 2;
}

//base_private u64_t _map_slot_of_idx(mem_heap_t *heap, usz_t idx)
//{
//  usz_t slot;
//  kernel_assert_d(idx % 2 == 0);
//
//  slot = (idx - _map_header_len() ) / 2;
//  kernel_assert_d(slot < _map_slot_max(heap));
//
//  return slot;
//}

base_private usz_t _map_slot_idx(mem_heap_t *heap, u64_t slot)
{
  kernel_assert_d(slot < _map_slot_max(heap));
  return _map_header_len() + slot * 2;
}

base_private void _map_slot_set_slot(
    mem_heap_t *heap, u64_t slot, u64_t set_slot)
{
  kernel_assert_d(slot < _map_slot_max(heap));
  kernel_assert_d(set_slot == _MAP_NULL || set_slot < _map_slot_max(heap));
  heap->map[_map_slot_idx(heap, slot)] = set_slot;
  heap->map[_map_slot_idx(heap, slot) + 1] = set_slot;
}

base_private void _map_slot_set_page(mem_heap_t *heap, u64_t slot, u64_t page)
{
  kernel_assert_d(slot < _map_slot_max(heap));
  kernel_assert_d(page == _MAP_NULL || page < _page_max(heap));
  heap->map[_map_slot_idx(heap, slot)] = page;
}

base_private void _map_slot_set_next(
    mem_heap_t *heap, u64_t slot, u64_t next /* Next slot */
)
{
  kernel_assert_d(slot < _map_slot_max(heap));
  kernel_assert_d(next == _MAP_NULL || next < _map_slot_max(heap));
  heap->map[_map_slot_idx(heap, slot) + 1] = next;
}

base_private usz_t _map_slot_get_page(mem_heap_t *heap, u64_t slot)
{
  usz_t idx;
  kernel_assert_d(slot < _map_slot_max(heap));

  idx = _map_slot_idx(heap, slot);
  return heap->map[idx];
}

base_private usz_t _map_slot_get_next(mem_heap_t *heap, u64_t slot)
{
  usz_t idx;
  kernel_assert_d(slot < _map_slot_max(heap));

  idx = _map_slot_idx(heap, slot);
  return heap->map[idx + 1];
}

base_private usz_t _map_slots_idx(void)
{
  return _CLASS_MAX * 2;
}

base_private u64_t _map_slots_dequeue(mem_heap_t *heap)
{
  usz_t head = _map_slots_idx();
  usz_t slot;
  usz_t next;

  kernel_assert_d(heap->map[head] != _MAP_NULL);
  kernel_assert_d(heap->map[head + 1] != _MAP_NULL);

  slot = heap->map[head];
  kernel_assert_d(slot < _map_slot_max(heap));

  next = _map_slot_get_next(heap, slot);
  kernel_assert_d(_map_slot_get_page(heap, slot) == next);

  if (next == _MAP_NULL) {
    kernel_assert_d(heap->map[head] == heap->map[head + 1]);
    heap->map[head] = _MAP_NULL;
    heap->map[head + 1] = _MAP_NULL;
  } else {
    kernel_assert_d(heap->map[head] != heap->map[head + 1]);
    heap->map[head] = next;
  }

  return next;
}

base_private void _map_slots_enqueue(mem_heap_t *heap, u64_t slot)
{
  usz_t head;

  kernel_assert_d(slot < _map_slot_max(heap));

  head = _map_slots_idx();

  if (heap->map[head] == _MAP_NULL) {
    kernel_assert_d(heap->map[head + 1] == _MAP_NULL);
    heap->map[head] = slot;
    heap->map[head + 1] = slot;
    _map_slot_set_page(heap, slot, _MAP_NULL);
    _map_slot_set_next(heap, slot, _MAP_NULL);
  } else {
    usz_t tail_slot;

    kernel_assert_d(heap->map[head + 1] != _MAP_NULL);

    tail_slot = heap->map[head + 1];
    kernel_assert_d(tail_slot < _map_slot_max(heap));
    kernel_assert_d(_map_slot_get_next(heap, tail_slot) == _MAP_NULL);
    kernel_assert_d(_map_slot_get_page(heap, tail_slot) == _MAP_NULL);
    _map_slot_set_page(heap, slot, _MAP_NULL);
    _map_slot_set_next(heap, slot, _MAP_NULL);
    _map_slot_set_slot(heap, tail_slot, slot);
  }
}

base_private usz_t _map_list_idx(u64_t list)
{
  kernel_assert_d(list < _CLASS_MAX);
  return list * 2;
}

base_private void _map_list_enqueue(mem_heap_t *heap, usz_t pg_0, usz_t class)
{
  usz_t head;
  usz_t slot;

  kernel_assert_d(class < _CLASS_MAX);
  /* Buddy allocation alignment. */
  kernel_assert_d(pg_0 % util_math_2_exp((u8_t) class) == 0);

  head = _map_list_idx(class);
  slot = _map_slots_dequeue(heap);
  _map_slot_set_page(heap, slot, pg_0);
  _map_slot_set_next(heap, slot, _MAP_NULL);

  if (heap->map[head] == _MAP_NULL) {
    kernel_assert_d(heap->map[head + 1] == _MAP_NULL);
    heap->map[head] = slot;
    heap->map[head + 1] = slot;
  } else {
    usz_t tail_slot;

    kernel_assert_d(heap->map[head + 1] != _MAP_NULL);

    tail_slot = heap->map[head + 1];
    kernel_assert_d(_map_slot_get_next(heap, tail_slot) == _MAP_NULL);
    _map_slot_set_next(heap, tail_slot, slot);
  }
}

/* Initialize heap map, returns the biggest heap size can be managed. */
base_private usz_t _map_init(mem_heap_t *heap, uptr_t heap_add, usz_t heap_size)
{
  usz_t size;
  usz_t cut;
  usz_t cut_pg;
  usz_t class;
  usz_t slot;

  kernel_assert(mem_align_check((uptr_t)(heap->map), PAGE_SIZE_VALUE_4K));
  kernel_assert(heap->map_size % PAGE_SIZE_VALUE_4K == 0);
  kernel_assert(heap->map_size > _map_header_len());
  kernel_assert(mem_align_check(heap_add, PAGE_SIZE_VALUE_4K));
  kernel_assert(heap_size % PAGE_SIZE_VALUE_4K == 0);

  size = _map_max_heap(heap->map_size);
  size = heap_size < size ? heap_size : size;
  kernel_assert_d(size > PAGE_SIZE_VALUE_4K);
  heap->add_0 = heap_add;
  heap->add_z = heap_add + size;

  for (class = 0; class < _CLASS_MAX; class ++) {
    heap->map[_map_list_idx(class)] = _MAP_NULL;
    heap->map[_map_list_idx(class) + 1] = _MAP_NULL;
  }

  heap->map[_map_slots_idx()] = _MAP_NULL;
  heap->map[_map_slots_idx() + 1] = _MAP_NULL;
  for (slot = 0; slot < _map_slot_max(heap); slot++) {
    _map_slots_enqueue(heap, slot);
  }

  cut = 0;
  while ((cut < size) && (size - cut >= PAGE_SIZE_VALUE_4K)) {
    class = util_math_log_2_down(size - cut);
    kernel_assert_d(class < _CLASS_MAX);
    cut_pg = cut / PAGE_SIZE_4K;
    kernel_assert_d(cut_pg % util_math_2_exp((u8_t) class) == 0);
    _map_list_enqueue(heap, cut_pg, class);

    cut += util_math_2_exp((u8_t) class) * PAGE_SIZE_VALUE_4K;
  }

  return size;
}

base_must_check mem_heap_t *mem_heap_new_bootstrap(
    uptr_t heap_add, usz_t heap_size)
{
  mem_heap_t *res;

  kernel_assert(boot_stage < MEM_BOOTSTRAP_STAGE_FINISH);
  kernel_assert(sizeof(mem_heap_t) < PAGE_SIZE_4K);

  if (!mem_align_check(heap_add, PAGE_SIZE_VALUE_4K)) {
    return NULL;
  }

  if (heap_size % PAGE_SIZE_VALUE_4K != 0) {
    return NULL;
  }

  res = (mem_heap_t *)_bootstrap_buff;
  res->map = (u64_t *)(_bootstrap_buff + PAGE_SIZE_4K);
  res->map_size = _BOOTSTRAP_BUFF_SIZE - PAGE_SIZE_4K;
  _map_init(res, heap_add, heap_size);

  return res;
}
