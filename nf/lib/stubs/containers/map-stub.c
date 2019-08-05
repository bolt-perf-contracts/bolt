#include <stdlib.h>
#include "klee/klee.h"
#include "lib/containers/map.h"
#include "map-stub-control.h"

char *prefix; /* For tracing */

__attribute__((noinline)) int map_allocate(map_keys_equality *keq, map_key_hash *khash,
                                           int capacity,
                                           struct Map **map_out)
{
  /* Tracing function and necessary variable(s) */
  klee_trace_ret();

  int allocation_succeeded = klee_int("map_allocation_succeeded");
  if (allocation_succeeded)
  {
    *map_out = malloc(sizeof(struct Map));
    klee_make_symbolic((*map_out), sizeof(struct Map), "map");
    klee_assert((*map_out) != NULL);

    memset((*map_out)->keys_present, 0, sizeof((*map_out)->keys_present));
    for (int n = 0; n < NUM_ELEMS; ++n)
    {
      (*map_out)->allocated_index[n] = 0;
      (*map_out)->key_deleted[n] = 0;
    }
    (*map_out)->keys_seen = 0;

    (*map_out)->capacity = capacity;
    (*map_out)->ent_cond = 0;
    (*map_out)->keq = keq;

    (*map_out)->Num_bucket_traversals = klee_int("Num_bucket_traversals");
    (*map_out)->Num_hash_collisions = klee_int("Num_hash_collisions");
    (*map_out)->occupancy = klee_range(0, capacity, "map_occupancy");

    /* Tracing other variable(s) */
    TRACE_VAL((uint32_t)(*map_out), "map", _u32)
    TRACE_VAR((*map_out)->capacity, "map_capacity")
    TRACE_VAR((*map_out)->occupancy, "map_occupancy")

    return 1;
  }
  return 0;
}

void map_reset(struct Map *map)
{
  //Do not trace. This function is an internal knob of the model.
  for (int n = 0; n < NUM_ELEMS; ++n)
  {
    map->allocated_index[n] = 0;
    map->key_deleted[n] = 0;
  }
  map->keys_seen = 0;
  map->occupancy = klee_range(0, map->capacity, "map_occupancy");
}

void map_set_key_size(struct Map *map, int size)
{
  map->key_size = size;
}

void map_set_entry_condition(struct Map *map, map_entry_condition *cond)
{
  map->ent_cond = cond;
}

__attribute__((noinline)) int map_get(struct Map *map, void *key, int *value_out)
{
  /* Tracing function and necessary variable(s) */
  klee_trace_ret();
  TRACE_VAL((uint32_t)(map), "map", _u32)
  TRACE_VAR(map->Num_bucket_traversals, "Num_bucket_traversals")
  TRACE_VAR(map->Num_hash_collisions, "Num_hash_collisions")

  /* For tracing map_has_this_key */
  int map_has_this_key;
  char *base = "map_has_this_key";
  char *suffix = "_occurence";
  char *final = malloc(strlen(base) + strlen(suffix) + 2); /* Final string of the form - map_has_this_key_occurence1 */
  klee_assert(final && "Failed malloc");
  strcpy(final, base);
  strcpy(&final[strlen(base)], suffix);
  for (int n = 0; n < map->keys_seen; ++n)
  {
    void *key_ptr = map->keys_present + n * map->key_size;
    if (map->keq(key, key_ptr))
    {
      if (map->key_deleted[n])
      {
        map_has_this_key = 0;
      }
      else
      {
        *value_out = map->allocated_index[n];
        map_has_this_key = 1;
      }
      final[strlen(base) + strlen(suffix)] = n + '0';
      final[strlen(base) + strlen(suffix) + 1] = '\0';
      TRACE_VAR(map_has_this_key, final)

      return map_has_this_key;
    }
  }
  klee_assert(map->keys_seen < NUM_ELEMS && "No space left in the map stub");
  map_has_this_key = klee_int("map_has_this_key");
  final[strlen(base) + strlen(suffix)] = map->keys_seen + '0';
  final[strlen(base) + strlen(suffix) + 1] = '\0';
  TRACE_VAR(map_has_this_key, final)
  if (map_has_this_key)
  {
    void *key_ptr = map->keys_present + map->keys_seen * map->key_size;
    memcpy(key_ptr, key, map->key_size);
    map->allocated_index[map->keys_seen] = klee_int("allocated_index");
    map->key_deleted[map->keys_seen] = 0;

    if (map->ent_cond)
    {
      klee_assume(map->ent_cond(key_ptr, map->allocated_index[map->keys_seen]));
    }
    *value_out = map->allocated_index[map->keys_seen];
    map->keys_seen++;
    return 1;
  }
  else
  {
    void *key_ptr = map->keys_present + map->keys_seen * map->key_size;
    memcpy(key_ptr, key, map->key_size);
    map->allocated_index[map->keys_seen] = klee_int("allocated_index");
    map->key_deleted[map->keys_seen] = 1;
    if (map->ent_cond)
    {
      klee_assume(map->ent_cond(key_ptr, map->allocated_index[map->keys_seen]));
    }
    map->keys_seen++;
    return 0;
  }
}

__attribute__((noinline)) void map_put(struct Map *map, void *key, int value)
{
  /* Tracing function and necessary variable(s) */
  klee_trace_ret();
  TRACE_VAL((uint32_t)(map), "map", _u32)
  TRACE_VAR(map->Num_bucket_traversals, "Num_bucket_traversals")
  TRACE_VAR(map->Num_hash_collisions, "Num_hash_collisions")

  if (map->ent_cond)
  {
    klee_assert(map->ent_cond(key, value));
  }
  map->occupancy += 1;
  for (int n = 0; n < map->keys_seen; ++n)
  {
    void *key_ptr = map->keys_present + n * map->key_size;
    if (map->keq(key, key_ptr))
    {
      klee_assert(map->key_deleted[n] && "Trying to insert already present key");
      map->key_deleted[n] = 0;
      map->allocated_index[n] = value;
      return;
    }
  }
  klee_assert(map->keys_seen < NUM_ELEMS && "No space left in the map stub");
  void *key_ptr = map->keys_present + map->keys_seen * map->key_size;
  memcpy(key_ptr, key, map->key_size);
  map->allocated_index[map->keys_seen] = value;
  map->key_deleted[map->keys_seen] = 0;
  map->keys_seen++;
}

__attribute__((noinline)) void map_erase(struct Map *map, void *key, void **trash)
{
  /* Tracing function and necessary variable(s) */
  klee_trace_ret();
  TRACE_VAL((uint32_t)(map), "map", _u32)
  TRACE_VAR(map->Num_bucket_traversals, "Num_bucket_traversals")
  TRACE_VAR(map->Num_hash_collisions, "Num_hash_collisions")

  for (int n = 0; n < map->keys_seen; ++n)
  {
    void *key_ptr = map->keys_present + n * map->key_size;
    if (map->keq(key, key_ptr))
    {
      map->occupancy -= 1;

      //It is important to differentiate the case
      // when that the key was deleted from the map,
      // as opposed to never existed on the first place.
      map->key_deleted[n] = 1;
      return; //The key is deleted, job's done
    }
  }
  klee_assert(map->keys_seen < NUM_ELEMS && "No space left in the map stub");
  // The key was not previously mentioned,
  // but we need to take a note that the key was deleted,
  // in case we access it in the future.
  void *key_ptr = map->keys_present + map->keys_seen * map->key_size;
  memcpy(key_ptr, key, map->key_size);
  map->key_deleted[map->keys_seen] = 1;
  map->keys_seen++;
}

__attribute__((noinline)) int map_size(struct Map *map)
{
  klee_trace_ret();
  return klee_int("map_size");
}
