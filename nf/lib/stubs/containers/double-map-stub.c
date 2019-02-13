#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "klee/klee.h"
#include "double-map-stub-control.h"

int Num_bucket_traversals_a;
int Num_bucket_traversals_b;
int Num_hash_collisions_a;
int Num_hash_collisions_b;

void dmap_set_entry_condition(struct DoubleMap* map, entry_condition* c) {
  //To avoid symbolic-pointer-dereference,
  // consciously trace "map" as a simple value.
  klee_trace_param_i32((uint32_t)map, "map");
  klee_trace_param_fptr(c, "c");
  ALLOW(map);
  map->ent_cond = c;
  DENY(map);
}

int calculate_str_size(struct str_field_descr* descr, int len) {
  int rez = 0;
  int sum = 0;
  for (int i = 0; i < len; ++i) {
    sum += descr[i].width;
    if (descr[i].offset + descr[i].width > rez)
      rez = descr[i].offset + descr[i].width;
  }
  klee_assert(rez == sum);
  return rez;
}

void dmap_set_layout(struct DoubleMap* map,
                     struct str_field_descr* key_a_field_, int key_a_count_,
                     struct str_field_descr* key_b_field_, int key_b_count_,
                     struct str_field_descr* value_field_, int value_count_,
                     struct nested_field_descr* value_nests_, int value_nests_count_) {
  ALLOW(map);
  klee_assert(key_a_count_ < prealloc_size);
  klee_assert(key_b_count_ < prealloc_size);
  klee_assert(value_count_ < prealloc_size);
  klee_assert(value_nests_count_ < prealloc_size);
  memcpy(map->key_a_fields, key_a_field_, sizeof(struct str_field_descr)*key_a_count_);
  memcpy(map->key_b_fields, key_b_field_, sizeof(struct str_field_descr)*key_b_count_);
  memcpy(map->value_fields, value_field_, sizeof(struct str_field_descr)*value_count_);
  memcpy(map->value_nests, value_nests_, sizeof(struct nested_field_descr)*value_nests_count_);
  map->key_a_field_count = key_a_count_;
  map->key_b_field_count = key_b_count_;
  map->value_field_count = value_count_;
  map->value_nests_count = value_nests_count_;
  map->key_a_size_g = calculate_str_size(key_a_field_, key_a_count_);
  map->key_b_size_g = calculate_str_size(key_b_field_, key_b_count_);
  DENY(map);
}

void dmap_increase_occupancy(struct DoubleMap* map, int change) {
  ALLOW(map);
  map->occupancy = map->occupancy + change;
  klee_assert(map->occupancy <= 65536);
  DENY(map);
}

void dmap_decrease_occupancy(struct DoubleMap* map, int change) {
  ALLOW(map);
  map->occupancy = map->occupancy - change;
  klee_assert(map->occupancy >= 0);
  DENY(map);
}

int* dmap_occupancy_p(struct DoubleMap* map) {
  return &map->occupancy;
}

void dmap_lowerbound_on_occupancy(struct DoubleMap* map, int lower_bound) {
  ALLOW(map);
  klee_assume(map->occupancy >= lower_bound);
  DENY(map);
}

int dmap_allocate(map_keys_equality eq_a,
                  map_key_hash hsh_a,
                  map_keys_equality eq_b,
                  map_key_hash hsh_b,
                  int value_size,
                  uq_value_copy v_cpy,
                  uq_value_destr v_destr,
                  dmap_extract_keys dexk,
                  dmap_pack_keys dpk,
                  int capacity,
                  int keys_capacity,
                  struct DoubleMap** map_out) {
  klee_trace_ret();
  klee_trace_param_fptr(eq_a, "eq_a");
  klee_trace_param_fptr(hsh_a, "hsh_a");
  klee_trace_param_fptr(eq_b, "eq_b");
  klee_trace_param_fptr(hsh_b, "hsh_b");
  klee_trace_param_i32(value_size, "value_size");
  klee_trace_param_fptr(v_cpy, "v_cpy");
  klee_trace_param_fptr(v_destr, "v_destr");
  klee_trace_param_fptr(dexk, "dexk");
  klee_trace_param_fptr(dpk, "dpk");
  klee_trace_param_i32(capacity, "capacity");
  klee_trace_param_i32(keys_capacity, "keys_capacity");
  klee_trace_param_ptr_directed(map_out, sizeof(struct DoubleMap*), "map_out", TD_OUT);
  int allocation_succeeded = klee_int("dmap_allocation_succeeded");
  *map_out = malloc(sizeof(struct DoubleMap));
  klee_possibly_havoc(*map_out, sizeof(struct DoubleMap), "map_out");
  klee_trace_extra_ptr(&allocation_succeeded, sizeof(allocation_succeeded),
                       "dmap_allocation_succeeded", "type", TD_BOTH);
  klee_trace_extra_ptr(&capacity, sizeof(capacity),
                       "dmap_capacity", "type", TD_BOTH);
  Num_bucket_traversals_a = klee_int("Num_bucket_traversals_a");
  Num_bucket_traversals_b = klee_int("Num_bucket_traversals_b");
  Num_hash_collisions_a = klee_int("Num_hash_collisions_a");
  Num_hash_collisions_b = klee_int("Num_hash_collisions_b");
  if (allocation_succeeded && *map_out != NULL) {
    memset(*map_out, 0, sizeof(struct DoubleMap));
    klee_assert(value_size < prealloc_size);

    (*map_out)->eq_a_g = eq_a;
    (*map_out)->eq_b_g = eq_b;
    (*map_out)->dexk_g = dexk;
    (*map_out)->dpk_g = dpk;
    (*map_out)->value_size_g = value_size;
    (*map_out)->has_this_key = klee_int("dmap_has_this_key");
    (*map_out)->entry_claimed = 0;
    //Need a separate memory record for allocated_indexp to be able
    // to mark it symbolic.
    (*map_out)->allocated_indexp = malloc(sizeof(*(*map_out)->allocated_indexp));
    klee_possibly_havoc((*map_out)->allocated_indexp, sizeof(*(*map_out)->allocated_indexp), "dmap_allocated_index");
    klee_make_symbolic((*map_out)->allocated_indexp,
                       sizeof(*(*map_out)->allocated_indexp),
                       "dmap_allocated_index");
    klee_assume(0 <= *(*map_out)->allocated_indexp);
    klee_assume(*(*map_out)->allocated_indexp < capacity);
    //Create a separate memory region to allow klee_make_symbolic
    (*map_out)->value = malloc(prealloc_size);
    klee_possibly_havoc((*map_out)->value, prealloc_size, "dmap_value");
    klee_make_symbolic((*map_out)->value, prealloc_size, "dmap_value");
    (*map_out)->occupancy = klee_range(0, capacity, "dmap_occupancy");
    (*map_out)->capacity = capacity;

    // Do not assume the ent_cond here, because depending on what comes next,
    // we may change the key_a, key_b or value. we assume the condition after
    // that change.

    DENY(*map_out);
    return 1;
  }
  return 0;
}

int dmap_get_a(struct DoubleMap* map, void* key, int* index) {
  ALLOW(map);
  klee_trace_ret();
  //To avoid symbolic-pointer-dereference,
  // consciously trace "map" as a simple value.
  klee_trace_param_i32((uint32_t)map, "map");
  klee_trace_extra_ptr(&map->has_this_key, sizeof(map->has_this_key),
                       "dmap_has_this_key", "type", TD_BOTH);
  klee_trace_extra_ptr(&map->occupancy, sizeof(map->occupancy),
                       "dmap_occupancy", "type", TD_BOTH);
  klee_trace_extra_ptr(&Num_bucket_traversals_a, sizeof(Num_bucket_traversals_a),
                       "Num_bucket_traversals_a", "type", TD_BOTH);
  klee_trace_extra_ptr(&Num_hash_collisions_a, sizeof(Num_hash_collisions_a),
                       "Num_hash_collisions_a", "type", TD_BOTH);
  klee_trace_param_ptr(key, map->key_a_size_g, "key");
  klee_trace_param_ptr(index, sizeof(int), "index");
  {
    for (int i = 0; i < map->key_a_field_count; ++i) {
      klee_trace_param_ptr_field(key,
                                 map->key_a_fields[i].offset,
                                 map->key_a_fields[i].width,
                                 map->key_a_fields[i].name);
    }
  }

  if (map->has_this_key) {
    klee_assert(!map->entry_claimed);
    void* key_a = NULL;
    void* key_b = NULL;
    map->dexk_g(map->value, &key_a, &key_b);

    klee_assume(map->eq_a_g(key_a, key));
    if (map->ent_cond)
      klee_assume(map->ent_cond(key_a, key_b,
                                *map->allocated_indexp, map->value));
    map->dpk_g(map->value, key_a, key_b);
    map->entry_claimed = 1;
    *index = *map->allocated_indexp;
    DENY(map);
    return 1;
  }
  DENY(map);
  return 0;
}

int dmap_get_b(struct DoubleMap* map, void* key, int* index) {
  ALLOW(map);
  klee_trace_ret();
  //To avoid symbolic-pointer-dereference,
  // consciously trace "map" as a simple value.
  klee_trace_param_i32((uint32_t)map, "map");
  klee_trace_extra_ptr(&map->has_this_key, sizeof(map->has_this_key),
                       "dmap_has_this_key", "type", TD_BOTH);
  klee_trace_extra_ptr(&map->occupancy, sizeof(map->occupancy),
                       "dmap_occupancy", "type", TD_BOTH);
  klee_trace_extra_ptr(&Num_bucket_traversals_b, sizeof(Num_bucket_traversals_b),
                       "Num_bucket_traversals_b", "type", TD_BOTH);
  klee_trace_extra_ptr(&Num_hash_collisions_b, sizeof(Num_hash_collisions_b),
                       "Num_hash_collisions_b", "type", TD_BOTH);
  klee_trace_param_ptr(key, map->key_b_size_g, "key");
  klee_trace_param_ptr(index, sizeof(int), "index");
  {
    for (int i = 0; i < map->key_b_field_count; ++i) {
      klee_trace_param_ptr_field(key,
                                 map->key_b_fields[i].offset,
                                 map->key_b_fields[i].width,
                                 map->key_b_fields[i].name);
    }
  }

  if (map->has_this_key) {
    klee_assert(!map->entry_claimed);
    void* key_a = NULL;
    void* key_b = NULL;
    map->dexk_g(map->value, &key_a, &key_b);
    klee_assume(map->eq_b_g(key_b, key));
    if (map->ent_cond)
      klee_assume(map->ent_cond(key_a, key_b,
                                *map->allocated_indexp, map->value));
    map->dpk_g(map->value, key_a, key_b);
    map->entry_claimed = 1;
    *index = *map->allocated_indexp;
    DENY(map);
    return 1;
  }
  DENY(map);
  return 0;
}

int dmap_put(struct DoubleMap* map, void* value_, int index) {
  ALLOW(map);
  klee_trace_ret();
  //To avoid symbolic-pointer-dereference,
  // consciously trace "map" as a simple value.
  klee_trace_param_i32((uint32_t)map, "map");
  klee_trace_extra_ptr(&map->has_this_key, sizeof(map->has_this_key),
                       "dmap_has_this_key", "type", TD_BOTH);
  klee_trace_extra_ptr(&map->occupancy, sizeof(map->occupancy),
                       "dmap_occupancy", "type", TD_BOTH);
  klee_trace_extra_ptr(&Num_bucket_traversals_a, sizeof(Num_bucket_traversals_a),
                       "Num_bucket_traversals_a", "type", TD_BOTH);
  klee_trace_extra_ptr(&Num_bucket_traversals_b, sizeof(Num_bucket_traversals_b),
                       "Num_bucket_traversals_b", "type", TD_BOTH);
  klee_trace_param_ptr(value_, map->value_size_g, "value_");
  klee_trace_param_i32(index, "index");
  {
    for (int i = 0; i < map->value_field_count; ++i) {
      klee_trace_param_ptr_field(value_,
                                 map->value_fields[i].offset,
                                 map->value_fields[i].width,
                                 map->value_fields[i].name);
    }
  }
  {
    for (int i = 0; i < map->value_nests_count; ++i) {
      klee_trace_param_ptr_nested_field(value_,
                                        map->value_nests[i].base_offset,
                                        map->value_nests[i].offset,
                                        map->value_nests[i].width,
                                        map->value_nests[i].name);
    }
  }

  // Can not ever fail, because index is guaranteed to point to the available
  // slot, therefore the map can not be full at this point.
  // Always returns 1.
  if (map->entry_claimed) {
    klee_assert(*map->allocated_indexp == index);
  }
  memcpy(map->value, value_, map->value_size_g);
  void* key_a = 0;
  void* key_b = 0;
  map->dexk_g(map->value, &key_a, &key_b);
  // This must be provided by the caller, since it his responsibility
  // to fulfill the value by the same index:
  klee_assert(map->ent_cond == NULL || map->ent_cond(key_a, key_b,
                                                     index,
                                                     map->value));
  klee_assume(map->occupancy < map->capacity);
  map->dpk_g(map->value, key_a, key_b);
  map->entry_claimed = 1;
  *map->allocated_indexp = index;
  DENY(map);
  dmap_increase_occupancy(map, 1);
  return 1;
}

int dmap_erase(struct DoubleMap* map, int index) {
  klee_assert(map != NULL);
  ALLOW(map);
  klee_trace_ret();
  //To avoid symbolic-pointer-dereference,
  // consciously trace "map" as a simple value.
  klee_trace_param_i32((uint32_t)map, "map");
  klee_trace_extra_ptr(&map->has_this_key, sizeof(map->has_this_key),
                       "dmap_has_this_key", "type", TD_BOTH);
  klee_trace_extra_ptr(&map->occupancy, sizeof(map->occupancy),
                       "dmap_occupancy", "type", TD_BOTH);
  klee_trace_extra_ptr(&Num_bucket_traversals_a, sizeof(Num_bucket_traversals_a),
                       "Num_bucket_traversals_a", "type", TD_BOTH);
  klee_trace_extra_ptr(&Num_bucket_traversals_b, sizeof(Num_bucket_traversals_b),
                       "Num_bucket_traversals_b", "type", TD_BOTH);
  klee_trace_extra_ptr(&Num_hash_collisions_a, sizeof(Num_hash_collisions_a),
                       "Num_hash_collisions_a", "type", TD_BOTH);
  klee_trace_extra_ptr(&Num_hash_collisions_b, sizeof(Num_hash_collisions_b),
                       "Num_hash_collisions_b", "type", TD_BOTH);
  klee_trace_param_i32(index, "index");

  dmap_decrease_occupancy(map, 1);
  klee_assert(0); //This model does not support erasure.
  DENY(map);
  return 0;
}

void dmap_get_value(struct DoubleMap* map, int index, void* value_out) {
  ALLOW(map);
  klee_trace_ret();
  //To avoid symbolic-pointer-dereference,
  // consciously trace "map" as a simple value.
  klee_trace_param_i32((uint32_t)map, "map");
  klee_trace_param_i32(index, "index");
  klee_trace_param_ptr_directed(value_out, map->value_size_g, "value_out", TD_OUT);
  {
    for (int i = 0; i < map->value_field_count; ++i) {
      klee_trace_param_ptr_field_directed(value_out,
                                          map->value_fields[i].offset,
                                          map->value_fields[i].width,
                                          map->value_fields[i].name,
                                          TD_OUT);
    }
  }
  {
    for (int i = 0; i < map->value_nests_count; ++i) {
      klee_trace_param_ptr_nested_field_directed(value_out,
                                                 map->value_nests[i].base_offset,
                                                 map->value_nests[i].offset,
                                                 map->value_nests[i].width,
                                                 map->value_nests[i].name,
                                                 TD_OUT);
    }
  }

  if (map->entry_claimed) {
    klee_assert(index == *map->allocated_indexp);
  } else {
    *map->allocated_indexp = index;
    map->entry_claimed = 1;
  }
  memcpy(value_out, map->value, map->value_size_g);
  DENY(map);
}

int dmap_size(struct DoubleMap* map) {
  klee_trace_ret();
  //To avoid symbolic-pointer-dereference,
  // consciously trace "map" as a simple value.
  klee_trace_param_i32((uint32_t)map, "map");
  klee_assert(0); //This model does not support size requests.
  return -1;
}

void dmap_reset(struct DoubleMap* map, int capacity) {
  ALLOW(map);
  map->entry_claimed = 0;
  map->occupancy = klee_range(0, capacity + 1, "dmap_occupancy");
  map->allocated_indexp = malloc(sizeof(int));
  klee_make_symbolic(map->allocated_indexp, sizeof(int), "allocated_index_reset");
  klee_assume(0 <= *map->allocated_indexp);
  klee_assume(*map->allocated_indexp < capacity);
  DENY(map);
}
