#ifdef BUILD_BUILTIN_TEST_ENABLED

base_private void _test_heap(void)
{
  usz_t total = 0;

  log_line_start(LOG_LEVEL_DEBUG);
  log_str(LOG_LEVEL_DEBUG, __FUNCTION__);
  log_str(LOG_LEVEL_DEBUG, " started..");
  log_line_end(LOG_LEVEL_DEBUG);

     
  for (usz_t size = 1; size < 1234; size += 13) {
    usz_t all_size;
    byte_t *mem;

    log_line_start(LOG_LEVEL_DEBUG);
    log_str(LOG_LEVEL_DEBUG, __FUNCTION__);
    log_uint_of_size(LOG_LEVEL_DEBUG, size);
    log_line_end(LOG_LEVEL_DEBUG);

 
    mem = mm_heap_alloc(size, &all_size);
    kernel_assert(all_size > size);
    total += all_size;
    for (usz_t by = 0; by < all_size; by++) {
      mem[by] = 0xAA;
    }
  }

  log_line_start(LOG_LEVEL_DEBUG);
  log_str(LOG_LEVEL_DEBUG, "Pass.");
  log_line_end(LOG_LEVEL_DEBUG);
}

#endif
