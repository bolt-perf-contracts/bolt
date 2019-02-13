#include <stdio.h>
#include <stdlib.h>
#include "lib/containers/map-impl.h"
#include "lib/containers/map.h"
#include "lib/flow.h"
#include "lib/flowmanager.h"
#include <limits.h>
#include <time.h>
#include <assert.h>
#include "x86intrin.h"
#define capacity 65536
#define max_size 41943040


/* This is a simple microbenchmark used to test the functionality of libVig datastructures */

int main()
{

        struct Map* map;
        map_allocate(int_key_eq,int_key_hash,capacity,&map);
	struct int_key *k1 = malloc(sizeof(*k1));
	int value = 1;
	int* val = malloc(sizeof *val);
 	void** b = malloc(sizeof(void*));
	k1->int_src_port = 1;
	k1->dst_port =1;
	k1->int_src_ip = 1;
	k1->dst_ip = 1;
	k1->int_device_id = 1;
	k1->protocol = 1;
	map_put(map,k1,value); /*Testing map_put() */ 
	int res = map_get(map,k1,val); /*Testing map_get() */
	map_erase(map,k1,b); /*Testing map_erase() */

	
	struct FlowManager* flow_manager;
	flow_manager  = allocate_flowmanager(0,0,0,0,0,capacity);
	struct flow flow;
	 /* This tests dchain_allocate_new_index() and dmap_put() */
	res=allocate_flow(flow_manager,k1,value,&flow);
	/* This tests dchain_rejuvenate_index() and dmap_get_a() */
        res=get_flow_by_int_key(flow_manager,k1,2*value,&flow);
	 /* This tests expire_items() which in turn tests dchain_expire_one_index and dmap_erase() */
        res=expire_flows(flow_manager,3*value);
}
	
