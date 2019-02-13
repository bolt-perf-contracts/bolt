#ifndef _DOUBLE_MAP_STUB_CONTROL_H_INCLUDED_
#define _DOUBLE_MAP_STUB_CONTROL_H_INCLUDED_
#include "include_ignored_by_verifast.h"
#include "str-descr.h"
#include "lib/containers/double-map.h"

#ifdef _NO_VERIFAST_
extern int Num_bucket_traversals_a;
extern int Num_bucket_traversals_b;
extern int Num_hash_collisions_a;
extern int Num_hash_collisions_b;
#endif//_NO_VERIFAST_

typedef int entry_condition(void* key_a, void* key_b, int index, void* value);

#define ALLOW(map) klee_allow_access((map), sizeof(struct DoubleMap))
#define DENY(map) klee_forbid_access((map), sizeof(struct DoubleMap), "Externally for the double map model code")

#define prealloc_size (256)

struct DoubleMap {
  int key_a_size_g;
  int key_b_size_g;
  int value_size_g;
  uint8_t *value;//[prealloc_size]; - for klee_make_symbolic
  int has_this_key;
  int entry_claimed;
  int* allocated_indexp; //must be a separate memory reg for klee_make_symbolic
  int occupancy;
  int capacity;

  entry_condition* ent_cond;
  struct str_field_descr key_a_fields[prealloc_size];
  struct str_field_descr key_b_fields[prealloc_size];
  struct str_field_descr value_fields[prealloc_size];
  struct nested_field_descr value_nests[prealloc_size];
  int key_a_field_count;
  int key_b_field_count;
  int value_field_count;
  int value_nests_count;

  map_keys_equality* eq_a_g;
  map_keys_equality* eq_b_g;
  dmap_extract_keys* dexk_g;
  dmap_pack_keys* dpk_g;
};

void dmap_set_entry_condition(struct DoubleMap* map, entry_condition* cond);
//@ requires true;
//@ ensures true;

void dmap_set_layout(struct DoubleMap* map,
                     struct str_field_descr* key_a_fields, int key_a_count,
                     struct str_field_descr* key_b_fields, int key_b_count,
                     struct str_field_descr* value_fields, int value_count,
                     struct nested_field_descr* value_nested_fields, int val_nests_count);
void dmap_reset(struct DoubleMap* map, int capacity);

void dmap_increase_occupancy(struct DoubleMap* map, int change);

void dmap_decrease_occupancy(struct DoubleMap* map, int change);

int* dmap_occupancy_p(struct DoubleMap* map);

void dmap_lowerbound_on_occupancy(struct DoubleMap* map, int lower_bound);

#endif//_DOUBLE_MAP_STUB_CONTROL_H_INCLUDED_
