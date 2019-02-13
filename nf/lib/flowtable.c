#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "lib/containers/double-map.h"
#include "flowtable.h"

#include "lib/nf_log.h"

#ifdef KLEE_VERIFICATION
#  include "lib/stubs/containers/double-map-stub-control.h"
#  include "lib/stubs/time_stub_control.h"
#endif //KLEE_VERIFICATION

void get_flow(struct DoubleMap* map, int index, struct flow* flow_out) {
  dmap_get_value(map, index, (char*)flow_out);
}

int get_flow_int(struct DoubleMap* map, struct int_key* key, int* index) {
    NF_DEBUG("look up for internal key key = ");
    log_int_key(key);
    return dmap_get_a(map, key, index);
}

int get_flow_ext(struct DoubleMap* map, struct ext_key* key, int* index) {
    NF_DEBUG("look up for external key key = ");
    log_ext_key(key);
    return dmap_get_b(map, key, index);
}

static inline void fill_int_key(struct flow *f, struct int_key *k) {
    k->int_src_port = f->int_src_port;
    k->dst_port = f->dst_port;
    k->int_src_ip = f->int_src_ip;
    k->dst_ip = f->dst_ip;
    k->int_device_id = f->int_device_id;
    k->protocol = f->protocol;
}

static inline void fill_ext_key(struct flow *f, struct ext_key *k) {
    k->ext_src_port = f->ext_src_port;
    k->dst_port = f->dst_port;
    k->ext_src_ip = f->ext_src_ip;
    k->dst_ip = f->dst_ip;
    k->ext_device_id = f->ext_device_id;
    k->protocol = f->protocol;
}


//Warning: this is thread-unsafe, do not youse more than 1 lcore!
int add_flow(struct DoubleMap* map, struct flow *f, int index) {
    NF_DEBUG("add_flow (f = ");
    log_flow(f);
    struct int_key* new_int_key = &f->ik;
    struct ext_key* new_ext_key = &f->ek;
    fill_int_key(f, new_int_key);
    fill_ext_key(f, new_ext_key);

//#define SWAP_BYTES(x) (((x&0xff) << 8) | ((x&0xff00) >> 8))
    //int nflows = dmap_size(map);
    //if (nflows % 0xff == 0)
    //  printf("%d flows, prts: %hu - %hu\n", nflows,
    //         SWAP_BYTES(f->int_src_port), SWAP_BYTES(f->dst_port));

    return dmap_put(map, f, index);
}

int allocate_flowtables(uint16_t nb_ports, int max_flows, struct DoubleMap** map_out) {
    (void)nb_ports;
    int alloc_result = dmap_allocate(int_key_eq, int_key_hash,
                                     ext_key_eq, ext_key_hash,
                                     sizeof(struct flow), flow_cpy,
                                     flow_destroy,
                                     flow_extract_keys,
                                     flow_pack_keys,
                                     max_flows,
                                     max_flows,
                                     map_out);
    if(alloc_result == 0) { // Allocation failure
        return 0;
    }

#ifdef KLEE_VERIFICATION
    dmap_set_layout(*map_out,
                    int_key_descrs, sizeof(int_key_descrs)/sizeof(struct str_field_descr),
                    ext_key_descrs, sizeof(ext_key_descrs)/sizeof(struct str_field_descr),
                    flow_descrs, sizeof(flow_descrs)/sizeof(struct str_field_descr),
                    flow_nests, sizeof(flow_nests)/sizeof(struct nested_field_descr));
#endif //KLEE_VERIFICATION

    return 1;
}

