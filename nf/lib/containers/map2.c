#include <stdlib.h>
#include "lib/containers/map-impl.h"
#include "map2.h"

#ifdef DUMP_PERF_VARS
#include "lib/nf_log.h"
#endif

struct Map {
  int* busybits;
  void** keyps;
  int* khs;
  int* chns;
  int* vals;
  int capacity;
  int size;
  map_keys_equality* keys_eq;
  map_key_hash* khash;
};

#ifndef NULL
#define NULL 0
#endif//NULL


int map2_allocate/*@ <t> @*/(map_keys_equality* keq, map_key_hash* khash,
                            int capacity,
                            struct Map** map_out)
{
  struct Map* old_map_val = *map_out;
  struct Map* map_alloc = malloc(sizeof(struct Map));
  if (map_alloc == NULL) return 0;
  *map_out = (struct Map*) map_alloc;
  int* bbs_alloc = malloc(sizeof(int)*capacity);
  if (bbs_alloc == NULL) {
    free(map_alloc);
    *map_out = old_map_val;
    return 0;
  }
  (*map_out)->busybits = bbs_alloc;
  void** keyps_alloc = malloc(sizeof(void*)*capacity);
  if (keyps_alloc == NULL) {
    free(bbs_alloc);
    free(map_alloc);
    *map_out = old_map_val;
    return 0;
  }
  (*map_out)->keyps = keyps_alloc;
  int* khs_alloc = malloc(sizeof(int)*capacity);
  if (khs_alloc == NULL) {
    free(keyps_alloc);
    free(bbs_alloc);
    free(map_alloc);
    *map_out = old_map_val;
    return 0;
  }
  (*map_out)->khs = khs_alloc;
  int* chns_alloc = malloc(sizeof(int)*capacity);
  if (chns_alloc == NULL) {
    free(khs_alloc);
    free(keyps_alloc);
    free(bbs_alloc);
    free(map_alloc);
    *map_out = old_map_val;
    return 0;
  }
  (*map_out)->chns = chns_alloc;
  int* vals_alloc = malloc(sizeof(int)*capacity);
  if (vals_alloc == NULL) {
    free(chns_alloc);
    free(khs_alloc);
    free(keyps_alloc);
    free(bbs_alloc);
    free(map_alloc);
    *map_out = old_map_val;
    return 0;
  }
  (*map_out)->vals = vals_alloc;
  (*map_out)->capacity = capacity;
  (*map_out)->size = 0;
  (*map_out)->keys_eq = keq;
  (*map_out)->khash = khash;
  map_impl_init((*map_out)->busybits,
                keq,
                (*map_out)->keyps,
                (*map_out)->khs,
                (*map_out)->chns,
                (*map_out)->vals,
                capacity);
  return 1;
}

int map2_get_1/*@ <t> @*/(struct Map* map, void* key, int* value_out)
{
#ifdef DUMP_PERF_VARS
 perf_dump_prefix="map2_";
 perf_dump_suffix="";
#endif

  map_key_hash* khash = map->khash;
  int hash = khash(key);
  return map_impl_get(map->busybits,
                      map->keyps,
                      map->khs,
                      map->chns,
                      map->vals,
                      key,
                      map->keys_eq,
                      hash,
                      value_out,
                      map->capacity);
}

void map2_put/*@ <t> @*/(struct Map* map, void* key, int value)
{
#ifdef DUMP_PERF_VARS
 perf_dump_prefix="map2_";
 perf_dump_suffix="";
#endif

  map_key_hash* khash = map->khash;
  int hash = khash(key);
  map_impl_put(map->busybits,
               map->keyps,
               map->khs,
               map->chns,
               map->vals,
               key, hash, value,
               map->capacity);
  ++map->size;
}


void map2_erase/*@ <t> @*/(struct Map* map, void* key, void** trash)
{
#ifdef DUMP_PERF_VARS
 perf_dump_prefix="map2_";
 perf_dump_suffix="";
#endif

  map_key_hash* khash = map->khash;
  int hash = khash(key);
  map_impl_erase(map->busybits,
                 map->keyps,
                 map->khs,
                 map->chns,
                 key,
                 map->keys_eq,
                 hash,
                 map->capacity,
                 trash);
  --map->size;
}

int map2_size/*@ <t> @*/(struct Map* map)
{
  return map->size;
}
