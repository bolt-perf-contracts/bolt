#include "lpm.h"

#include <stdlib.h>
#include <rte_ethdev.h>

#include <klee/klee.h>


void __attribute__((noinline))
lpm_init(const char fname[], void **lpm_out) {
  *lpm_out = malloc(1);
  klee_trace_ret();

}

uint32_t __attribute__((noinline))
lpm_lookup(void *lpm, uint32_t addr) {
  klee_trace_ret();
  int num_stages_uv =  klee_int("num_stages_in_LUT") >0 ? 2:1; 
  klee_trace_extra_ptr(&num_stages_uv, sizeof(num_stages_uv), "lpm_stages", "type", TD_BOTH);

  return klee_range(0, rte_eth_dev_count(), "lpm_next_hop");
}
