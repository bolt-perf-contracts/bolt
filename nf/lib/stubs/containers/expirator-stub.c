#include <klee/klee.h>
#include "lib/expirator.h"
#include "lib/stubs/containers/double-chain-stub-control.h"
#include "lib/stubs/containers/double-map-stub-control.h"
#include "lib/stubs/containers/map-stub-control.h"

int expire_items(struct DoubleChain *chain, struct DoubleMap *map,
                 time_t time)
{
  ALLOW(map);
  klee_trace_ret();
  int *occupancyp = dmap_occupancy_p(map);
  klee_trace_extra_ptr(occupancyp, sizeof(*occupancyp),
                       "dmap_occupancy", "type", TD_BOTH);
  DENY(map);
  klee_trace_extra_ptr(&Num_bucket_traversals, sizeof(Num_bucket_traversals), "Num_bucket_traversals", "type", TD_BOTH);
  klee_trace_extra_ptr(&Num_hash_collisions, sizeof(Num_hash_collisions), "Num_hash_collisions", "type", TD_BOTH);
  klee_trace_extra_ptr(&recent_flow, sizeof(recent_flow),
                       "recent_flow", "type", TD_BOTH);
  klee_trace_param_i32((uint32_t)chain, "chain");
  klee_trace_param_i32((uint32_t)map, "map");
  klee_trace_param_i64(time, "exp_time");
  int nfreed = klee_int("number_of_freed_flows");
  klee_trace_extra_ptr(&nfreed, sizeof(nfreed), "expired_flows", "type", TD_BOTH);
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
  klee_trace_param_i32((uint32_t)chain, "chain");
  klee_trace_param_i32((uint32_t)vector, "vector");
  klee_trace_param_i32((uint32_t)map, "map");
  klee_trace_param_i64(time, "time");
  int nfreed = klee_int("number_of_freed_flows");
  klee_assume(0 <= nfreed);
  if (0 < nfreed)
  {
    dchain_make_space(chain);
  }
  klee_trace_extra_ptr(&map->occupancy, sizeof(map->occupancy), "map_occupancy", "type", TD_BOTH);
  klee_trace_extra_ptr(&map->Num_bucket_traversals, sizeof(map->Num_bucket_traversals), "Num_bucket_traversals", "type", TD_BOTH);
  klee_trace_extra_ptr(&map->Num_hash_collisions, sizeof(map->Num_hash_collisions), "Num_hash_collisions", "type", TD_BOTH);
  klee_trace_extra_ptr(&nfreed, sizeof(nfreed), "expired_flows", "type", TD_BOTH);
  return nfreed;
}