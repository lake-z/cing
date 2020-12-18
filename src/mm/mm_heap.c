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

#ifdef BUILD_BUILTIN_TEST_ENABLED
/* Built-in tests declarations */
base_private void _test_verify_all_list_class(void);
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
  blk_size = _size_of_class(class);

  /* blk_base is the address of the original block allocated from paging system,
     from which we do binary buddy splitting/coalescing */
  blk_base = (uptr_t)blk - VADD_HEAP_START;
  blk_base = mm_align_down(blk_base, _size_of_class(_BLOCK_MAX_CLASS - 1));
  blk_offs = (uptr_t)blk - VADD_HEAP_START - blk_base;
  kernel_assert_d(mm_align_check(blk_offs, blk_size));

  /* Binary buddy invariant: block offset to base should alwyas align with
     block size. */
  if (mm_align_check(blk_offs - blk_size, blk_size * 2)) {
    buddy_addr = blk_offs - blk_size;
  } else {
    kernel_assert_d(mm_align_check(blk_offs, blk_size * 2));
    buddy_addr = blk_offs + blk_size;
  }
  buddy_block = (block_t *)(buddy_addr + blk_base + VADD_HEAP_START);

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
    bo_t ok = mm_frame_get(&frame_pa);

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
      kernel_assert_d(succ);
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

base_private void _coalescing_block(block_t *block)
{
  block_t *buddy;

  _block_validate(block);
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
    _coalescing_block(block);
  } else {
    _free_list_enqueue(block, block->class);
  }
}

byte_t *mm_heap_alloc(usz_t len, usz_t *all_len)
{
  u8_t free_class;
  block_t *free;
  bo_t ok;

  kernel_assert_d(len > 0);
  kernel_assert(len < _size_of_class(_BLOCK_MAX_CLASS - 1));

  free_class = (u8_t)util_math_log_2_up(len + sizeof(block_t));
  if (free_class <= PAGE_SIZE_VALUE_LOG_4K) {
    free_class = 0;
  } else {
    free_class = (u8_t)(free_class - PAGE_SIZE_VALUE_LOG_4K);
  }
  free = _free_list_dequeue(free_class);

  if (free == NULL) {
    ok = _split_from((u8_t)(free_class + 1));
    if (ok) {
      free = _free_list_dequeue(free_class);
      kernel_assert_d(free != NULL);
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

void mm_heap_free(byte_t *block_user)
{
  block_t *block = _block_check_in(block_user);
  _coalescing_block(block);
}

void mm_heap_init(void)
{
  for (usz_t i = 0; i < _BLOCK_MAX_CLASS; i++) {
    _free_list[i] = NULL;
  }
  _heap_end = VADD_HEAP_START;
}

base_private void _heap_validate(void)
{
  block_t *blk;
  usz_t size;

  blk = (block_t *)VADD_HEAP_START;
  while (((uptr_t)blk) < _heap_end) {
    kernel_assert(mm_align_check((uptr_t)blk, PAGE_SIZE_VALUE_4K));
    _block_validate(blk);

    size = _size_of_class(blk->class);
    kernel_assert(mm_align_check((uptr_t)blk - VADD_HEAP_START, size));
    blk = blk + size;
  }
}

#ifdef BUILD_BUILTIN_TEST_ENABLED
/* Built-in tests */
base_private void _test_verify_all_list_class(void)
{
  for (u8_t class = 0; class < _BLOCK_MAX_CLASS; class ++) {
    block_t *free = _free_list[class];
    while (free != NULL) {
      kernel_assert(free->class == class);
      free = free->next_free;
    }
  }
}

base_private void _test_basic_ops(void)
{
  usz_t all_size;
  byte_t *mem;
  
  mem = mm_heap_alloc(1, &all_size);

  kernel_assert(mem != NULL);
  kernel_assert(all_size >= 1);

  mm_heap_free(mem);
}

void test_heap(void)
{
  usz_t total;
  byte_t *mems[BYTE_MAX];
  usz_t mem_lens[BYTE_MAX];
  usz_t mems_cnt;
  usz_t mems_cap = 1;

  _test_basic_ops();

  kernel_assert_d(mems_cap < BYTE_MAX);

  mems_cnt = 0;
  total = 0;
  for (usz_t size = 1; size < 1024 * 1024; size += 1357) {
    usz_t all_size;
    byte_t *mem;

    _heap_validate();

    mem = mm_heap_alloc(size, &all_size);
    kernel_assert(all_size >= size);
    if (mems_cnt >= mems_cap) {
      kernel_assert_d(mems_cnt == mems_cap);
      for (usz_t mem_idx = 0; mem_idx < mems_cap; mem_idx++) {
        mem = mems[mem_idx];
        for (usz_t by = 0; by < mem_lens[mem_idx]; by++) {
          kernel_assert(mem[by] == (byte_t)mem_idx);
        }
        mm_heap_free(mem);
        for (usz_t mem_idx_after = mem_idx + 1; mem_idx_after < mems_cap;
             mem_idx_after++) {
          mem = mems[mem_idx_after];
          for (usz_t by = 0; by < mem_lens[mem_idx_after]; by++) {
            mem = mems[mem_idx_after];
          }
        }
      }
      mems_cnt = 0;
      total = 0;
    }

    kernel_assert_d(mems_cnt < mems_cap);
    mems[mems_cnt] = mem;
    mem_lens[mems_cnt] = all_size;
    total += all_size;
    for (usz_t by = 0; by < all_size; by++) {
      mem[by] = (byte_t)mems_cnt;
    }
    mems_cnt++;
    for (usz_t mem_idx = 0; mem_idx < mems_cnt; mem_idx++) {
      mem = mems[mem_idx];
      for (usz_t by = 0; by < mem_lens[mem_idx]; by++) {
        kernel_assert(mem[by] == (byte_t)mem_idx);
      }
    }
  }

  _test_verify_all_list_class();
  log_builtin_test_pass();
}
#endif
