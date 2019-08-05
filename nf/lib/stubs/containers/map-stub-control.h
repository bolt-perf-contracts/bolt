#ifndef _MAP_STUB_CONTROL_H_INCLUDED_
#define _MAP_STUB_CONTROL_H_INCLUDED_

#include "lib/containers/map.h"
#include <stdbool.h>
#include "str-descr.h"
#include <string.h>

#define PREALLOC_SIZE (256)
#define NUM_ELEMS (3)

typedef bool map_entry_condition(void *key, int index);

struct Map
{
  /* Storing keys, values */
  char keys_present[NUM_ELEMS * PREALLOC_SIZE]; /* Array storing all keys map has seen */
  int allocated_index[NUM_ELEMS];               /* Value for each key */
  int key_deleted[NUM_ELEMS];                   /* 1 in nth position implies nth key has been deleted */
  int keys_seen;                                /* Number of unique keys seen by the map at any point in time*/

  /* Map config */
  int capacity;
  int key_size;
  map_entry_condition *ent_cond;
  map_keys_equality *keq;

  /* Symbolic state */
  int Num_bucket_traversals;
  int Num_hash_collisions;
  int occupancy; /* This is different from keys_seen, see map_get code */
};

void map_set_key_size(struct Map *map, int size);

void map_set_entry_condition(struct Map *map, map_entry_condition *cond);

void map_reset(struct Map *map);

#endif //_MAP_STUB_CONTROL_H_INCLUDED_
