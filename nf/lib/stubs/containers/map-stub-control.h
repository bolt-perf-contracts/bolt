#ifndef _MAP_STUB_CONTROL_H_INCLUDED_
#define _MAP_STUB_CONTROL_H_INCLUDED_

#include "lib/containers/map.h"
#include "str-descr.h"

#define PREALLOC_SIZE (256)
#define NUM_ELEMS (3)

typedef int map_entry_condition(void* key, int index);

struct Map {
  void* keyp[NUM_ELEMS];
  char key_storage[PREALLOC_SIZE*NUM_ELEMS];
  int allocated_index[NUM_ELEMS];
  int key_deleted[NUM_ELEMS];
  int next_unclaimed_entry;
  int capacity;
  int occupancy;
  int backup_occupancy;
  int has_layout;
  int key_size;
  int key_field_count;
  int nested_key_field_count;
  map_entry_condition* ent_cond;
  struct str_field_descr key_fields[PREALLOC_SIZE];
  struct nested_field_descr key_nests[PREALLOC_SIZE];
  char* key_type;
  map_keys_equality* keq;

  int Num_bucket_traversals;
  int Num_hash_collisions;
};

void map_set_layout(struct Map* map,
                    struct str_field_descr* key_fields,
                    int key_fields_count,
                    struct nested_field_descr* key_nests,
                    int nested_key_fields_count,
                    char* key_type);

void map_set_entry_condition(struct Map* map, map_entry_condition* cond);

void map_reset(struct Map* map);

void map_increase_occupancy(struct Map* map,int change);

void map_decrease_occupancy(struct Map* map, int change);

#endif//_MAP_STUB_CONTROL_H_INCLUDED_
