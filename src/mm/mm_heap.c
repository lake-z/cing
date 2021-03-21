#include "kernel_panic.h"
#include "log.h"
#include "mm_private.h"
#include "util.h"

/*
 * TODO:
 * Currently free lists are organized as single linked lists, which makes
 * searching a given block when doing coalesce expensive. Optimize this
 * later.
 */

#define _BLOCK_MAX_CLASS 16

typedef struct block block_t;
typedef struct block {
  u8_t class;
  bo_t is_free;
  block_t *next_free;
  u32_t magic;
} block_t;

base_private block_t *_free_list[_BLOCK_MAX_CLASS];
base_private uptr_t _heap_end;

#ifdef BUILD_SELF_TEST_ENABLED

/* Built-in tests declarations */
base_private void _test_helper_verify_all_list_class(void);
base_private void _test_helper_verify_all_block_coalesced(void);

#endif

base_private u64_t _size_of_class(usz_t class)
{
  kernel_assert_d(class < _BLOCK_MAX_CLASS);
  return util_math_2_exp((u8_t) class) * PAGE_SIZE_VALUE_4K;
}

base_private void _block_init(
    block_t *block, u8_t class, bo_t free, block_t *next)
{
  kernel_assert_d(mm_align_check((uptr_t)block, PAGE_SIZE_VALUE_4K));
  block->class = class;
  block->is_free = free;
  block->next_free = next;
  block->magic = 0xBABEFACE;
}

base_private void _block_validate(block_t *block)
{
  kernel_assert_d(mm_align_check((uptr_t)block, PAGE_SIZE_VALUE_4K));
  kernel_assert_d(block->magic == 0xBABEFACE);
}

base_private byte_t *_block_check_out(block_t *block, usz_t *payload_len)
{
  kernel_assert_d(mm_align_check((uptr_t)block, PAGE_SIZE_VALUE_4K));
  kernel_assert_d(block->class < _BLOCK_MAX_CLASS);
  kernel_assert_d(block->is_free == true);
  block->is_free = false;
  usz_t len = _size_of_class(block->class);
  (*payload_len) = len - sizeof(block_t);
  return (byte_t *)(((uptr_t)block) + sizeof(block_t));
}

base_private block_t *_block_check_in(byte_t *block_user)
{
  block_t *block = (block_t *)((uptr_t)block_user - sizeof(block_t));
  _block_validate(block);
  kernel_assert_d(block->is_free == false);
  block->is_free = true;
  return block;
}

base_private block_t *_block_find_buddy(block_t *blk)
{
  u8_t class;
  uptr_t blk_base;
  usz_t blk_size;
  uptr_t blk_offs;
  uptr_t buddy_addr;
  block_t *buddy_block;

  kernel_assert_d(mm_align_check((uptr_t)blk, PAGE_SIZE_VALUE_4K));
  class = blk->class;
  kernel_assert_d(class < _BLOCK_MAX_CLASS);
  blk_size = _size_of_class(class);

  /* blk_base is the address of the original block allocated from paging system,
     from which we do binary buddy splitting/coalescing */
  blk_base = (uptr_t)blk - VA_48_HEAP;
  blk_base = mm_align_down(blk_base, _size_of_class(_BLOCK_MAX_CLASS - 1));

  /* Block offset relative to base */
  blk_offs = (uptr_t)blk - VA_48_HEAP - blk_base;
  kernel_assert_d(mm_align_check(blk_offs, blk_size));

  /* Binary buddy invariant: block offset to base should alwyas align with
     block size. */
  if (mm_align_check(blk_offs - blk_size, blk_size * 2)) {
    buddy_addr = blk_offs - blk_size;
  } else {
    kernel_assert_d(mm_align_check(blk_offs, blk_size * 2));
    buddy_addr = blk_offs + blk_size;
  }
  buddy_block = (block_t *)(buddy_addr + blk_base + VA_48_HEAP);

  /* Buddy meta must always be eixsted as we have already splitted. */
  _block_validate(buddy_block);
  kernel_assert_d((buddy_block->class) <= (blk->class));
  return buddy_block;
}

base_private block_t *_free_list_dequeue(u8_t class)
{
  block_t *blk;

  kernel_assert_d(class < _BLOCK_MAX_CLASS);

  blk = _free_list[class];

  if (blk != NULL) {
    _block_validate(blk);
    kernel_assert_d(blk->class == class);
    _free_list[class] = blk->next_free;
  }

  return blk;
}

/* Dequeue the given block from it's containing free list. */
base_private void _free_list_dequeue_block(block_t *block)
{
  block_t *free;
  bo_t found;

  _block_validate(block);

  found = false;
  free = _free_list[block->class];
  while (free != NULL) {
    if (free == block) {
      kernel_assert_d(free == _free_list[block->class]);
      _free_list[block->class] = block->next_free;
      found = true;
      break;
    } else if (free->next_free == block) {
      free->next_free = block->next_free;
      found = true;
      break;
    } else {
      free = free->next_free;
    }
  }
  kernel_assert_d(found == true);
}

base_private bo_t _free_list_is_empty(u8_t class)
{
  kernel_assert_d(class < _BLOCK_MAX_CLASS);
  return _free_list[class] == NULL;
}

/* Initialize free block fields, and after that enqueue the corresponding
   free list. */
base_private void _free_list_enqueue(block_t *free, u8_t class)
{
  kernel_expect(class < _BLOCK_MAX_CLASS);
  _block_init(free, class, true, _free_list[class]);
  _free_list[class] = free;
  kernel_ensure(free->class == class);
}

/*
base_private u64_t _size_of_heap(void)
{
  return _heap_end - VADD_HEAP_START;
}
*/

/* Always expand heap at a step of the same size */
base_private bo_t _expand_heap(void)
{
  u64_t size = _size_of_class(_BLOCK_MAX_CLASS - 1);
  uptr_t heap_add;
  block_t *free;

  kernel_assert_d(size > 0);
  kernel_assert_d((size % PAGE_SIZE_VALUE_4K) == 0);
  kernel_assert_d(_free_list_is_empty(_BLOCK_MAX_CLASS - 1));
  for (heap_add = 0; heap_add < size; heap_add += PAGE_SIZE_VALUE_4K) {
    uptr_t frame_pa = UPTR_NULL;
    bo_t ok = mm_frame_alloc(&frame_pa);

    kernel_assert_d(ok);
    kernel_assert_d(frame_pa != UPTR_NULL);

    ok = mm_page_map(_heap_end + heap_add, frame_pa);
    kernel_assert_d(ok == true);
  }

  free = (block_t *)_heap_end;
  _free_list_enqueue(free, _BLOCK_MAX_CLASS - 1);
  _heap_end += size;

  return true;
}

/* Split free block on given class, and insert the 2 buddies into lower class */
base_private bo_t _split_from(u8_t class)
{
  block_t *free;
  bo_t succ;

  kernel_assert_d(class < _BLOCK_MAX_CLASS);
  kernel_assert_d(class > 0);

  free = _free_list_dequeue(class);
  if (free == NULL) {
    if ((class + 1) < _BLOCK_MAX_CLASS) {
      succ = _split_from((u8_t)(class + 1));
      if (succ) {
        free = _free_list_dequeue(class);
        kernel_assert_d(free != NULL);
      }
    } else {
      kernel_assert_d(class == (_BLOCK_MAX_CLASS - 1));
      succ = _expand_heap();
      kernel_assert(succ);
      free = _free_list_dequeue(class);
      kernel_assert_d(free != NULL);
    }
  } else {
    succ = true;
  }

  if (succ) {
    usz_t next_size = _size_of_class((u8_t)(class - 1));

    kernel_assert_d(free != NULL);
    kernel_assert_d((free->class) == class);
    kernel_assert_d(_free_list[class - 1] == NULL);

    _free_list_enqueue(free, (u8_t)(class - 1));

    free = (block_t *)(((uptr_t)free) + next_size);
    _free_list_enqueue(free, (u8_t)(class - 1));
  }

  return succ;
}

base_private block_t *_coalescing_block(block_t *block)
{
  block_t *buddy;

  _block_validate(block);

  if (block->class < (_BLOCK_MAX_CLASS - 1)) {
    buddy = _block_find_buddy(block);
    if ((buddy->class == block->class) && (buddy->is_free)) {
      _free_list_dequeue_block(buddy);

      /* Destroy coalesced block meta */
      if ((uptr_t)buddy < (uptr_t)block) {
        mm_clean(block, sizeof(block_t));
        block = buddy;
      } else {
        mm_clean(buddy, sizeof(block_t));
      }

      _block_init(block, (u8_t)(block->class + 1), true, NULL);
      block = _coalescing_block(block);
    }
  }

  return block;
}

vptr_t mm_heap_alloc(usz_t len, usz_t *all_len)
{
  u8_t free_class;
  block_t *free;
  bo_t ok;

  kernel_assert_d(len > 0);
  kernel_assert(
      (len + sizeof(block_t)) <= _size_of_class(_BLOCK_MAX_CLASS - 1));

  free_class = (u8_t)util_math_log_2_up(len + sizeof(block_t));
  if (free_class <= PAGE_SIZE_VALUE_LOG_4K) {
    free_class = 0;
  } else {
    free_class = (u8_t)(free_class - PAGE_SIZE_VALUE_LOG_4K);
  }
  kernel_assert(free_class < _BLOCK_MAX_CLASS);

  free = _free_list_dequeue(free_class);

  if (free == NULL) {
    if (free_class < (_BLOCK_MAX_CLASS - 1)) {
      ok = _split_from((u8_t)(free_class + 1));
      if (ok) {
        free = _free_list_dequeue(free_class);
        kernel_assert_d(free != NULL);
      }
    } else {
      ok = _expand_heap();
      kernel_assert(ok);
      free = _free_list_dequeue(free_class);
      kernel_assert(free != NULL);
    }
  } else {
    ok = true;
  }

  if (ok) {
    kernel_assert_d(free->class == free_class);
    kernel_assert_d(free != NULL);
    free = (block_t *)_block_check_out(free, all_len);
    kernel_assert_d((*all_len) >= len);
  } else {
    kernel_assert_d(free == NULL);
  }

  return (byte_t *)free;
}

vptr_t mm_heap_alloc_minimum(usz_t *all_len)
{
  return mm_heap_alloc(1, all_len);
}

void mm_heap_free(vptr_t block_user)
{
  block_t *block = _block_check_in(block_user);
  block = _coalescing_block(block);
  _free_list_enqueue(block, block->class);
}

void mm_heap_bootstrap(void)
{
  for (usz_t i = 0; i < _BLOCK_MAX_CLASS; i++) {
    _free_list[i] = NULL;
  }
  _heap_end = VA_48_HEAP;
}

#ifdef BUILD_SELF_TEST_ENABLED

base_private void _heap_validate(void)
{
  block_t *blk;
  usz_t size;

  blk = (block_t *)VA_48_HEAP;
  while (((uptr_t)blk) < _heap_end) {
    kernel_assert(mm_align_check((uptr_t)blk, PAGE_SIZE_VALUE_4K));
    _block_validate(blk);

    size = _size_of_class(blk->class);
    kernel_assert(mm_align_check((uptr_t)blk - VA_48_HEAP, size));
    blk = blk + size;
  }
}

/* Built-in tests */
base_private void _test_helper_verify_all_list_class(void)
{
  for (u8_t class = 0; class < _BLOCK_MAX_CLASS; class ++) {
    block_t *free = _free_list[class];
    while (free != NULL) {
      kernel_assert(free->class == class);
      free = free->next_free;
    }
  }
}

base_private void _test_helper_verify_all_block_coalesced(void)
{
  for (u8_t class = 0; class < (_BLOCK_MAX_CLASS - 1); class ++) {
    kernel_assert(_free_list[class] == NULL);
  }
  kernel_assert(_free_list[_BLOCK_MAX_CLASS - 1] != NULL);
  _block_validate(_free_list[_BLOCK_MAX_CLASS - 1]);
  kernel_assert(
      _free_list[_BLOCK_MAX_CLASS - 1]->class == (_BLOCK_MAX_CLASS - 1));

  /* Enable this after implemented heap shrink
       kernel_assert(_free_list[_BLOCK_MAX_CLASS - 1]->next_free == NULL); */
}

base_private void _test_alloc_then_free(void)
{
  usz_t all_size;
  byte_t *mem[1024];
  usz_t test_size[1024];
  usz_t test_size_cnt = 0;

  test_size[test_size_cnt++] = 1;
  test_size[test_size_cnt++] = 1358;
  test_size[test_size_cnt++] = 2715;
  test_size[test_size_cnt++] = 4072;
  test_size[test_size_cnt++] = 5429;
  test_size[test_size_cnt++] = 4096;
  test_size[test_size_cnt++] = 10000;
  test_size[test_size_cnt++] = 1024 * 1024;
  test_size[test_size_cnt++] = 123 * 1024 * 1024;
  test_size[test_size_cnt++] =
      _size_of_class(_BLOCK_MAX_CLASS - 1) - sizeof(block_t);

  for (usz_t consecutive = 1; consecutive <= test_size_cnt; consecutive++) {
    for (usz_t test = 0; test < test_size_cnt;) {
      usz_t alloc = 0;

      for (; (alloc < consecutive) && (test < test_size_cnt); alloc++) {
        mem[alloc] = mm_heap_alloc(test_size[test], &all_size);
        kernel_assert(mem[alloc] != NULL);
        kernel_assert(all_size >= test_size[test]);
        mm_clean(mem[alloc], all_size);
        test++;

        _test_helper_verify_all_list_class();
      }

      for (usz_t free = 0; free < alloc; free++) {
        mm_heap_free(mem[free]);

        _test_helper_verify_all_list_class();
        _heap_validate();
      }

      _test_helper_verify_all_block_coalesced();
    }
  }

  log_builtin_test_pass();
}

base_private void _test_random_alloc_free(usz_t op_total, usz_t mems_cap)
{
  byte_t *mem;
  byte_t *mems[BYTE_MAX];
  usz_t mems_lens[BYTE_MAX];
  usz_t mems_cnt;
  usz_t free_cnt;
  u64_t op;
  usz_t len;
  u64_t len_class;
  const usz_t len_max = _size_of_class(_BLOCK_MAX_CLASS - 1) - sizeof(block_t);

  kernel_assert_d(mems_cap < BYTE_MAX);

  mems_cnt = 0;
  free_cnt = 0;
  len = 0;
  len_class = 0;
  for (usz_t i = 0; i < op_total; i++) {
    if (mems_cnt >= mems_cap) {
      kernel_assert_d(mems_cnt == mems_cap);
      op *= 2; /* A free operation */
    } else if (mems_cnt == 0) {
      op = op * 2 + 1; /* A alloc operation */
    } else {
      op = util_rand_int_next(op);
    }

    /* Verify all bytes saved in allocated memories are untouched,
     * and then free. */
    if ((op % 2) == 0) {
      kernel_assert_d(mems_cnt > 0);
      kernel_assert_d(mems_cnt <= mems_cap);

      free_cnt = util_rand_int_next(free_cnt);
      free_cnt = free_cnt % mems_cnt + 1;

      for (usz_t free_idx = 0; free_idx < free_cnt; free_idx++) {
        usz_t mem_idx = mems_cnt - free_idx - 1;

        kernel_assert(mem_idx < mems_cnt);
        mem = mems[mem_idx];
        for (usz_t by = 0; by < mems_lens[mem_idx]; by++) {
          kernel_assert(mem[by] == (byte_t)mem_idx);
        }

        mm_heap_free(mem);
      }
      mems_cnt -= free_cnt;
      kernel_assert(mems_cnt < mems_cap);
    } else {
      usz_t all_len;
      usz_t len_class_max;

      kernel_assert_d(mems_cnt < mems_cap);

      len_class = util_rand_int_next(len_class);
      len_class_max = len_class % 4;

      switch (len_class_max) {
      case 0:
        len_class_max = 32 * 1024;
        break;
      case 1:
        len_class_max = 512 * 1024;
        break;
      case 2:
        len_class_max = 8 * 1024 * 1024;
        break;
      case 3:
        len_class_max = len_max;
        break;
      default:
        kernel_panic("IMPOSSIBLE");
      }

      len = util_rand_int_next(len);
      len = len % len_class_max + 1;

      mem = mm_heap_alloc(len, &all_len);
      kernel_assert(all_len >= len);

      /* Save allocated memory, actual allocated length, and fill them as known
       * bytes. */
      mems[mems_cnt] = mem;
      mems_lens[mems_cnt] = all_len;
      for (usz_t by = 0; by < (all_len); by++) {
        mem[by] = (byte_t)mems_cnt;
      }
      mems_cnt++;
    }

    _heap_validate();
    _test_helper_verify_all_list_class();
  }

  for (usz_t i = 0; i < mems_cnt; i++) {
    mem = mems[i];
    for (usz_t by = 0; by < mems_lens[i]; by++) {
      kernel_assert(mem[by] == (byte_t)i);
    }

    mm_heap_free(mem);
  }
  _test_helper_verify_all_block_coalesced();

  log_builtin_test_pass();
}

void test_heap(void)
{
  _test_alloc_then_free();
  _test_random_alloc_free(200, 5);
}
#endif
