#include "kernel_panic.h"
#include "util.h"

base_private const u64_t RAND_1 = 151117737;
base_private const u64_t RAND_2 = 119785373;
base_private const u64_t RAND_3 = 85689495;
base_private const u64_t RAND_4 = 76595339;
base_private const u64_t RAND_SUM_2 = 98781234;
base_private const u64_t RAND_SUM_3 = 126792457;
base_private const u64_t RAND_SUM_4 = 63498502;
base_private const u64_t RAND_XOR_1 = 187678878;
base_private const u64_t RAND_XOR_2 = 143537923;

u64_t util_rand_int_next(u64_t curr)
{
  usz_t bit_cnt = 8 * sizeof(u64_t);

  curr = RAND_2 * curr + RAND_SUM_3;
  curr = RAND_XOR_1 ^ curr;
  curr = (curr << 20) + (curr >> (bit_cnt - 20));
  curr = RAND_3 * curr + RAND_SUM_4;
  curr = RAND_XOR_2 ^ curr;
  curr = (curr << 20) + (curr >> (bit_cnt - 20));
  curr = RAND_1 * curr + RAND_SUM_2;

  return curr;
}
