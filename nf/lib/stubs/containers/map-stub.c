#include <stdlib.h>
#include <string.h>
#include "klee/klee.h"
#include "lib/containers/map.h"
#include "map-stub-control.h"

int has_this_key_a;
int has_this_key_b;
int map_allocate(map_keys_equality* keq, map_key_hash* khash,
                 int capacity,
                 struct Map** map_out) {
  klee_trace_ret();
  klee_trace_param_fptr(keq, "keq");
  klee_trace_param_fptr(khash, "khash");
  klee_trace_param_i32(capacity, "capacity");
  klee_trace_param_ptr(map_out, sizeof(struct Map*), "map_out");
  int allocation_succeeded = klee_int("map_allocation_succeeded");
  klee_trace_extra_ptr(&allocation_succeeded, sizeof(allocation_succeeded),
                       "map_allocation_succeeded", "type", TD_BOTH);
  klee_trace_extra_ptr(&capacity, sizeof(capacity),
                       "map_capacity", "type", TD_BOTH);
  if (allocation_succeeded) {
    *map_out = malloc(sizeof(struct Map));
    klee_possibly_havoc(*map_out, sizeof(struct Map), "map");
    klee_assume(*map_out != NULL);
    klee_make_symbolic((*map_out), sizeof(struct Map), "map");
    klee_assert((*map_out) != NULL);
    for (int n = 0; n < NUM_ELEMS; ++n) {
      (*map_out)->keyp[n] = NULL;
      (*map_out)->key_deleted[n] = 0;
    }
    (*map_out)->next_unclaimed_entry = 0;
    (*map_out)->keq = keq;
    (*map_out)->capacity = capacity;
    (*map_out)->has_layout = 0;
    (*map_out)->ent_cond = 0;
    (*map_out)->occupancy = klee_range(0, capacity, "map_occupancy");
    (*map_out)->backup_occupancy = klee_range(0, capacity, "map_backup_occupancy");
    (*map_out)->Num_bucket_traversals = klee_int("Num_bucket_traversals");
    (*map_out)->Num_hash_collisions = klee_int("Num_hash_collisions");
    return 1;
  }
  return 0;
}

void map_reset(struct Map* map) {
  //Do not trace. This function is an internal knob of the model.
  for (int n = 0; n < NUM_ELEMS; ++n) {
    map->keyp[n] = NULL;
    map->key_deleted[n] = 0;
    map->allocated_index[n] = klee_int("map_allocated_index");
  }
  map->next_unclaimed_entry = 0;
  map->occupancy = map->backup_occupancy;
}

static
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

void map_set_layout(struct Map* map,
                    struct str_field_descr* key_fields,
                    int key_fields_count,
                    struct nested_field_descr* key_nests,
                    int nested_key_fields_count,
                    char* key_type) {
  //Do not trace. This function is an internal knob of the model.
  klee_assert(key_fields_count < PREALLOC_SIZE);
  klee_assert(nested_key_fields_count < PREALLOC_SIZE);
  memcpy(map->key_fields, key_fields,
         sizeof(struct str_field_descr)*key_fields_count);
  if (0 < nested_key_fields_count) {
    memcpy(map->key_nests, key_nests,
           sizeof(struct nested_field_descr)*nested_key_fields_count);
  }
  map->key_field_count = key_fields_count;
  map->nested_key_field_count = nested_key_fields_count;
  map->key_size = calculate_str_size(key_fields,
                                     key_fields_count);
  klee_assert(map->key_size < PREALLOC_SIZE);
  map->has_layout = 1;
  map->key_type = key_type;
}

void map_set_entry_condition(struct Map* map, map_entry_condition* cond) {
  map->ent_cond = cond;
}


#define TRACE_KEY_FIELDS(key, map)                                      \
  {                                                                     \
    for (int i = 0; i < map->key_field_count; ++i) {                    \
      klee_trace_param_ptr_field(key,                                   \
                                 map->key_fields[i].offset,             \
                                 map->key_fields[i].width,              \
                                 map->key_fields[i].name);              \
    }                                                                   \
    for (int i = 0; i < map->nested_key_field_count; ++i) {            \
      klee_trace_param_ptr_nested_field(key,                            \
                                        map->key_nests[i].base_offset,  \
                                        map->key_nests[i].offset,       \
                                        map->key_nests[i].width,        \
                                        map->key_nests[i].name);        \
    }                                                                   \
  }

void map_increase_occupancy(struct Map* map, int change) {
  map->occupancy = map->occupancy + change;
}

void map_decrease_occupancy(struct Map* map, int change) {
  map->occupancy = map->occupancy - change;
}

int map_get_1(struct Map* map, void* key, int* value_out) {
  klee_trace_ret();
  klee_trace_extra_ptr(&(map->occupancy), sizeof(map->occupancy),
                       "map_occupancy", "type", TD_BOTH);
  klee_trace_extra_ptr(&(map->backup_occupancy), sizeof(map->backup_occupancy),
                       "map_backup_occupancy", "type", TD_BOTH);
  klee_trace_extra_ptr(&(map->Num_bucket_traversals),
                       sizeof(map->Num_bucket_traversals),
                       "Num_bucket_traversals", "type", TD_BOTH);
  klee_trace_extra_ptr(&(map->Num_hash_collisions),
                       sizeof(map->Num_hash_collisions),
                       "Num_hash_collisions", "type", TD_BOTH);
  klee_assert(map->has_layout);
  //To avoid symbolic-pointer-dereference,
  // consciously trace "map" as a simple value.
  klee_trace_param_i32((uint32_t)map, "map");

  klee_make_symbolic(&(has_this_key_a), sizeof(has_this_key_a), "map_has_this_key_a");
  klee_trace_extra_ptr(&(has_this_key_a), sizeof(has_this_key_a), "map_has_this_key_a", "type", TD_BOTH);

  klee_trace_param_tagged_ptr(key, map->key_size, "key", map->key_type, TD_BOTH);
  klee_trace_param_ptr(value_out, sizeof(int), "value_out");
  TRACE_KEY_FIELDS(key, map);
  for (int n = 0; n < map->next_unclaimed_entry; ++n) {
    if (map->keq(key, map->keyp[n])) {
      if (map->key_deleted[n]) {
        klee_assume(has_this_key_a == 0);
        return 0;
      } else {
        *value_out = map->allocated_index[n];
        klee_assume(has_this_key_a == 1);
        return 1;
      }
    }
  }
  klee_assert(map->next_unclaimed_entry < NUM_ELEMS && "No space left in the map stub");
  if (has_this_key_a) {
    int n = map->next_unclaimed_entry;
    ++map->next_unclaimed_entry;
    map->key_deleted[n] = 0;

    klee_assert(0 < map->key_size);
    klee_assert(map->key_size < PREALLOC_SIZE);
    void* stored_keyp = map->key_storage + map->key_size*n;
    memcpy(stored_keyp, key, map->key_size);
    map->keyp[n] = stored_keyp;

    map->allocated_index[n] = klee_int("allocated_index");
    if (map->ent_cond) {
      klee_assume(map->ent_cond(map->keyp[n], map->allocated_index[n]));
    }
    *value_out = map->allocated_index[n];
    return 1;
  } else {
    return 0;
  }
}

int map_get_2(struct Map* map, void* key, int* value_out) {
  klee_trace_ret();
  klee_trace_extra_ptr(&(map->occupancy), sizeof(map->occupancy),
                       "map_occupancy", "type", TD_BOTH);
  klee_trace_extra_ptr(&(map->backup_occupancy), sizeof(map->backup_occupancy),
                       "map_backup_occupancy", "type", TD_BOTH);
  klee_trace_extra_ptr(&(map->Num_bucket_traversals),
                       sizeof(map->Num_bucket_traversals),
                       "Num_bucket_traversals", "type", TD_BOTH);
  klee_trace_extra_ptr(&(map->Num_hash_collisions),
                       sizeof(map->Num_hash_collisions),
                       "Num_hash_collisions", "type", TD_BOTH);
  klee_assert(map->has_layout);
  //To avoid symbolic-pointer-dereference,
  // consciously trace "map" as a simple value.
  klee_trace_param_i32((uint32_t)map, "map");

  klee_make_symbolic(&(has_this_key_b), sizeof(has_this_key_b), "map_has_this_key_b");
  klee_trace_extra_ptr(&(has_this_key_b), sizeof(has_this_key_b), "map_has_this_key_b", "type", TD_BOTH);

  klee_trace_param_tagged_ptr(key, map->key_size, "key", map->key_type, TD_BOTH);
  klee_trace_param_ptr(value_out, sizeof(int), "value_out");
  TRACE_KEY_FIELDS(key, map);
  for (int n = 0; n < map->next_unclaimed_entry; ++n) {
    if (map->keq(key, map->keyp[n])) {
      if (map->key_deleted[n]) {
        klee_assume(has_this_key_b == 0);
        return 0;
      } else {
        *value_out = map->allocated_index[n];
        klee_assume(has_this_key_b == 1);
        return 1;
      }
    }
  }
  klee_assert(map->next_unclaimed_entry < NUM_ELEMS && "No space left in the map stub");
  if (has_this_key_b) {
    int n = map->next_unclaimed_entry;
    ++map->next_unclaimed_entry;
    map->key_deleted[n] = 0;

    klee_assert(0 < map->key_size);
    klee_assert(map->key_size < PREALLOC_SIZE);
    void* stored_keyp = map->key_storage + map->key_size*n;
    memcpy(stored_keyp, key, map->key_size);
    map->keyp[n] = stored_keyp;

    map->allocated_index[n] = klee_int("allocated_index");
    if (map->ent_cond) {
      klee_assume(map->ent_cond(map->keyp[n], map->allocated_index[n]));
    }
    *value_out = map->allocated_index[n];
    return 1;
  } else {
    return 0;
  }
}

void map_put(struct Map* map, void* key, int value) {
  klee_trace_ret();
  klee_trace_extra_ptr(&(map->occupancy), sizeof(map->occupancy),
                       "map_occupancy", "type", TD_BOTH);
  klee_trace_extra_ptr(&(map->Num_bucket_traversals),
                       sizeof(map->Num_bucket_traversals),
                       "Num_bucket_traversals", "type", TD_BOTH);
  klee_assert(map->has_layout);
  // VVV allow potential overflow here, validator should catch it if it really occurs.
  //klee_assert(map->occupancy < map->capacity);
  //To avoid symbolic-pointer-dereference,
  // consciously trace "map" as a simple value.
  klee_trace_param_i32((uint32_t)map, "map");
  klee_trace_extra_ptr(&(map->occupancy), sizeof(map->occupancy),
                       "map_occupancy", "type", TD_BOTH);
  klee_trace_extra_ptr(&(map->backup_occupancy), sizeof(map->backup_occupancy),
                       "map_backup_occupancy", "type", TD_BOTH);

  klee_trace_param_tagged_ptr(key, map->key_size, "key", map->key_type, TD_BOTH);
  klee_trace_param_i32(value, "value");
  TRACE_KEY_FIELDS(key, map);
  if (map->ent_cond) {
    klee_assert(map->ent_cond(key, value));
  }
  map_increase_occupancy(map,1);
  for (int n = 0; n < map->next_unclaimed_entry; ++n) {
    if (map->keq(key, map->keyp[n])) {
      klee_assert(map->key_deleted[n] && "Duplicate key, otherwise");
      map->key_deleted[n] = 0;
      map->allocated_index[n] = value;
      return;//Undeleted a key -> like inserted one, no need for another one.
    }
  }
  klee_assert(map->next_unclaimed_entry < NUM_ELEMS && "No space left in the map stub");
  int n = map->next_unclaimed_entry;
  ++map->next_unclaimed_entry;
  map->key_deleted[n] = 0;
  klee_assert(0 < map->key_size);
  klee_assert(map->key_size < PREALLOC_SIZE);
  void* stored_keyp = map->key_storage + map->key_size*n;
  memcpy(stored_keyp, key, map->key_size);
  map->keyp[n] = stored_keyp;
  map->allocated_index[n] = value;
}

void map_erase(struct Map* map, void* key, void** trash) {
  klee_trace_ret();
  klee_trace_extra_ptr(&(map->occupancy), sizeof(map->occupancy),
                       "map_occupancy", "type", TD_BOTH);
  klee_trace_extra_ptr(&(map->backup_occupancy), sizeof(map->backup_occupancy),
                       "map_backup_occupancy", "type", TD_BOTH);
  klee_trace_extra_ptr(&(map->Num_bucket_traversals),
                       sizeof(map->Num_bucket_traversals),
                       "Num_bucket_traversals", "type", TD_BOTH);
  klee_trace_extra_ptr(&(map->Num_hash_collisions),
                       sizeof(map->Num_hash_collisions),
                       "Num_hash_collisions", "type", TD_BOTH);
  klee_assert(map->has_layout);
  //To avoid symbolic-pointer-dereference,
  // consciously trace "map" as a simple value.
  klee_trace_param_i32((uint32_t)map, "map");
  klee_trace_extra_ptr(&(map->occupancy), sizeof(map->occupancy),
                       "map_occupancy", "type", TD_BOTH);
  klee_trace_extra_ptr(&(map->backup_occupancy), sizeof(map->backup_occupancy),
                       "map_backup_occupancy", "type", TD_BOTH);

  klee_trace_param_tagged_ptr(key, map->key_size, "key", map->key_type, TD_BOTH);
  TRACE_KEY_FIELDS(key, map);
  klee_trace_param_ptr(trash, sizeof(void*), "trash");

  map_decrease_occupancy(map,1);
  for (int n = 0; n < map->next_unclaimed_entry; ++n) {
    if (map->keq(key, map->keyp[n])) {
      //It is important to differentiate the case
      // when that the key was deleted from the map, 
      // as opposed to never existed on the first place.
      map->key_deleted[n] = 1;
      return;//The key is deleted, job's done
    }
  }
  assert(map->next_unclaimed_entry < NUM_ELEMS && "No more space in the map stub");
  // The key was not previously mentioned,
  // but we need to take a note that the key was deleted,
  // in case we access it in the future.
  int n = map->next_unclaimed_entry;
  ++map->next_unclaimed_entry;
  klee_assert(0 < map->key_size);
  klee_assert(map->key_size < PREALLOC_SIZE);
  void* stored_keyp = map->key_storage + map->key_size*n;
  memcpy(stored_keyp, key, map->key_size);
  map->keyp[n] = stored_keyp;
  map->key_deleted[n] = 1;
}

int map_size(struct Map* map) {
  klee_trace_ret();
  //To avoid symbolic-pointer-dereference,
  // consciously trace "map" as a simple value.
  klee_trace_param_i32((uint32_t)map, "map");
  klee_assert(0); //No support for retreiving size for now.
}
