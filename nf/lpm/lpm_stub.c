#include "lpm.h"

#include <stdlib.h>
#include <rte_ethdev.h>

#include <klee/klee.h>

char *prefix; /* For tracing */

void __attribute__((noinline))
lpm_init(const char fname[], void **lpm_out)
{
  *lpm_out = malloc(1);
  klee_trace_ret();
}

uint32_t __attribute__((noinline))
lpm_lookup(void *lpm, uint32_t addr)
{
  klee_trace_ret();
  int multi_stage_lookup = klee_int("num_stages_in_LUT");
  PERF_MODEL_BRANCH(multi_stage_lookup, 1, 2)
  TRACE_VAR(multi_stage_lookup, "multi_stage_lookup")
  return klee_range(0, rte_eth_dev_count(), "lpm_next_hop");
}
