#include <klee/klee.h>
#include "lib/expirator.h"
#include "lib/stubs/containers/double-chain-stub-control.h"
#include "lib/stubs/containers/double-map-stub-control.h"
#include "lib/stubs/containers/map-stub-control.h"

char *prefix; /* For tracing */
int expire_items(struct DoubleChain *chain, struct DoubleMap *map,
                 time_t time)
{
  ALLOW(map);
  klee_trace_ret();
  int *occupancyp = dmap_occupancy_p(map);
  DENY(map);

  TRACE_VAL((uint32_t)(map), "dmap", _u32)
  TRACE_VAR(Num_bucket_traversals, "Num_bucket_traversals")
  TRACE_VAR(Num_hash_collisions, "Num_hash_collisions")
  TRACE_VAR(recent_flow, "recent_flow")

  int nfreed = klee_int("number_of_freed_flows");
  TRACE_VAR(nfreed, "expired_flows")

  klee_assume(0 <= nfreed);
  dmap_lowerbound_on_occupancy(map, nfreed);
  if (nfreed > 0)
  {
    dchain_make_space(chain);
    PERF_MODEL_BRANCH(recent_flow, 1, 0)
  }
  //Tell dchain model that we freed some indexes here
  dmap_decrease_occupancy(map, nfreed);
  return nfreed;
}

int expire_items_single_map(struct DoubleChain *chain,
                            struct Vector *vector,
                            struct Map *map,
                            time_t time)
{
  klee_trace_ret();
  TRACE_VAL((uint32_t)(map), "map", _u32)

  int nfreed = klee_int("number_of_freed_flows");
  klee_assume(0 <= nfreed);
  if (0 < nfreed)
  {
    dchain_make_space(chain);
  }
  TRACE_VAR(map->Num_bucket_traversals, "Num_bucket_traversals")
  TRACE_VAR(map->Num_hash_collisions, "Num_hash_collisions")
  TRACE_VAR(nfreed, "expired_flows")

  return nfreed;
}