#include <klee/klee.h>
#include "lib/expirator.h"
#include "lib/stubs/containers/double-chain-stub-control.h"
#include "lib/stubs/containers/double-map-stub-control.h"
#include "lib/stubs/containers/map-stub-control.h"

int expire_items(struct DoubleChain* chain, struct DoubleMap* map,
                 time_t time) {
  ALLOW(map);
  klee_trace_ret();
  int *occupancyp = dmap_occupancy_p(map);
  klee_trace_extra_ptr(occupancyp, sizeof(*occupancyp),
                       "dmap_occupancy", "type", TD_BOTH);
  DENY(map);
  klee_trace_extra_ptr(&Num_bucket_traversals_a,sizeof(Num_bucket_traversals_a),"Num_bucket_traversals_a","type",TD_BOTH);
  klee_trace_extra_ptr(&Num_bucket_traversals_b,sizeof(Num_bucket_traversals_b),"Num_bucket_traversals_b","type",TD_BOTH);
  klee_trace_extra_ptr(&Num_hash_collisions_a,sizeof(Num_hash_collisions_a),"Num_hash_collisions_a","type",TD_BOTH);
  klee_trace_extra_ptr(&Num_hash_collisions_b,sizeof(Num_hash_collisions_b),"Num_hash_collisions_b","type",TD_BOTH);
  klee_trace_param_i32((uint32_t)chain, "chain");
  klee_trace_param_i32((uint32_t)map, "map");
  klee_trace_param_i64(time, "exp_time");
  int nfreed = klee_int("number_of_freed_flows");
  klee_trace_extra_ptr(&nfreed,sizeof(nfreed),"expired_flows","type",TD_BOTH);
  klee_assume(0 <= nfreed);
  dmap_lowerbound_on_occupancy(map, nfreed);
  if (nfreed > 0) {
    dchain_make_space(chain);
  }
  //Tell dchain model that we freed some indexes here
  dmap_decrease_occupancy(map, nfreed);
  return nfreed;
}

int expire_items_single_map(struct DoubleChain* chain,
                            struct Vector* vector,
                            struct Map* map,
                            time_t time) {
  klee_trace_ret();
  klee_trace_param_i32((uint32_t)chain, "chain");
  klee_trace_param_i32((uint32_t)vector, "vector");
  klee_trace_param_i32((uint32_t)map, "map");
  klee_trace_param_i64(time, "time");
  int nfreed = klee_int("number_of_freed_flows");
  klee_assume(0 <= nfreed);
  if (0 < nfreed) {
    dchain_make_space(chain);
  }
  klee_trace_extra_ptr(&map->occupancy,sizeof(map->occupancy),"map_occupancy","type",TD_BOTH);
  klee_trace_extra_ptr(&map->Num_bucket_traversals,sizeof(map->Num_bucket_traversals),"Num_bucket_traversals","type",TD_BOTH);
  klee_trace_extra_ptr(&map->Num_hash_collisions,sizeof(map->Num_hash_collisions),"Num_hash_collisions","type",TD_BOTH);
  klee_trace_extra_ptr(&nfreed,sizeof(nfreed),"expired_flows","type",TD_BOTH);
  return nfreed;
}

int expire_items_single_map2(struct DoubleChain* chain,
                             struct Vector* vector,
                             struct Map* map,
                             time_t time) {
  klee_trace_ret();
  klee_trace_param_i32((uint32_t)chain, "chain");
  klee_trace_param_i32((uint32_t)vector, "vector");
  klee_trace_param_i32((uint32_t)map, "map");
  klee_trace_param_i64(time, "time");
  int nfreed = klee_int("number_of_freed_flows2");
  klee_assume(0 <= nfreed);
  if (0 < nfreed) {
    dchain_make_space(chain);
  }
  klee_trace_extra_ptr(&map->occupancy,sizeof(map->occupancy),"map2_occupancy","type",TD_BOTH);
  klee_trace_extra_ptr(&map->Num_bucket_traversals,sizeof(map->Num_bucket_traversals),"Num_bucket_traversals2","type",TD_BOTH);
  klee_trace_extra_ptr(&map->Num_hash_collisions,sizeof(map->Num_hash_collisions),"Num_hash_collisions2","type",TD_BOTH);
  klee_trace_extra_ptr(&nfreed,sizeof(nfreed),"expired_flows2","type",TD_BOTH);
  return nfreed;
}
