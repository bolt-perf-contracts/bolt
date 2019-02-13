#ifndef _MAP2_H_INCLUDED_
#define _MAP2_H_INCLUDED_

#include "map-impl.h"
#include "map-util.h"

struct Map;


int map2_allocate/*@ <t> @*/(map_keys_equality* keq,
                            map_key_hash* khash, int capacity,
                            struct Map** map_out);

int map2_get_1/*@ <t> @*/(struct Map* map, void* key, int* value_out);

void map2_put/*@ <t> @*/(struct Map* map, void* key, int value);

void map2_erase/*@ <t> @*/(struct Map* map, void* key, void** trash);

int map2_size/*@ <t> @*/(struct Map* map);

#endif//_MAP2_H_INCLUDED_
