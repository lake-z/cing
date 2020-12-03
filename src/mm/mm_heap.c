#include "kernel_panic.h"
#include "mm_private.h"
#include "util.h"

#define _BLOCK_MAX_CLASS 16

typedef struct block_free block_free_t;
struct block_free {
  block_free_t *next;
};

base_private block_free_t *_free_list[_BLOCK_MAX_CLASS];
base_private uptr_t _heap_end;

void mm_heap_init(void)
{
  for (usz_t i = 0; i < _BLOCK_MAX_CLASS; i++) {
    _free_list[i] = NULL;
  }
  _heap_end = VADD_HEAP_START;
}

base_private u64_t _size_of_class(usz_t class)
{
  kernel_assert_d(class < _BLOCK_MAX_CLASS);
  return util_math_2_exp((u8_t) class) * PAGE_SIZE_VALUE_4K;
}

/*
base_private u64_t _size_of_heap(void)
{
  return _heap_end - VADD_HEAP_START;
}
*/

base_private bo_t _expand_heap(void)
{
  u64_t size = _size_of_class(_BLOCK_MAX_CLASS - 1);
  uptr_t heap_add;
  block_free_t *free;

  kernel_assert_d(size > 0);
  kernel_assert_d((size % PAGE_SIZE_VALUE_4K) == 0);
  for (heap_add = 0; heap_add < size; heap_add += PAGE_SIZE_VALUE_4K) {
    uptr_t frame_pa = UPTR_NULL;
    bo_t ok = mm_frame_get(&frame_pa);

    kernel_assert_d(ok);
    kernel_assert_d(frame_pa != UPTR_NULL);
    ok = mm_page_map(_heap_end + heap_add, frame_pa);
    kernel_assert_d(ok == true);
  }

  free = (block_free_t *)_heap_end;
  free->next = _free_list[_BLOCK_MAX_CLASS - 1];
  _free_list[_BLOCK_MAX_CLASS - 1] = free;

  _heap_end += size;

  return true;
}

base_private bo_t _free_from(usz_t class)
{
  block_free_t *free;
  bo_t succ;

  kernel_assert_d(class < _BLOCK_MAX_CLASS);
  kernel_assert_d(class > 0);

  free = _free_list[class];
  if (free == NULL) {
    if ((class + 1) < _BLOCK_MAX_CLASS) {
      succ = _free_from(class + 1);
      if (succ) {
        free = _free_list[class];
        kernel_assert_d(free != NULL);
      }
    } else {
      kernel_assert_d(class == (_BLOCK_MAX_CLASS - 1));
      succ = _expand_heap();
      kernel_assert_d(succ);
      free = _free_list[class];
      kernel_assert_d(free != NULL);
    }
  } else {
    succ = true;
  }

  if (succ) {
    usz_t next_size = _size_of_class(class - 1);
    kernel_assert_d(free != NULL);
    kernel_assert_d(_free_list[class - 1] == NULL);
    free->next = _free_list[class - 1];
    _free_list[class - 1] = free;
    free = (block_free_t *)(((uptr_t)free) + next_size);
    free->next = _free_list[class - 1];
    _free_list[class - 1] = free;
  }

  return succ;
}

byte_t *mm_heap_alloc(usz_t len, usz_t *all_len)
{
  usz_t free_class;
  block_free_t *free;
  bo_t ok;

  kernel_assert_d(len > 0);
  kernel_assert(len < _size_of_class(_BLOCK_MAX_CLASS - 1));

  free_class = util_math_log_2_up(len);
  free = _free_list[free_class];

  if (free == NULL) {
    ok = _free_from(free_class + 1);
    if (ok) {
      free = _free_list[free_class];
      kernel_assert_d(free != NULL);
    }
  } else {
    ok = true;
  }

  if (ok) {
    kernel_assert_d(free != NULL);
    _free_list[free_class] = free->next;
    *all_len = _size_of_class(free_class);
  } else {
    kernel_assert_d(free == NULL);
  }
  return (byte_t *)free;
}
