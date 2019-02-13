#include <string.h>
#include <stdio.h>
#include <time.h>
#include "x86intrin.h"

#include "vignat/flowmanager.h"
#include "vignat/flowtable.h"
#include "lib/containers/double-map.h"
#include "lib/containers/double-chain-impl.h"
#include "lib/containers/double-chain.h"

#define NUM_FLOWS 65535
#define EXP_TIME 10
//#define LOG
#define REPORT_INCREMENT 5


// From flow.h
int int_key_hash(void* key)
{
  struct int_key* ik = key;
  return ik->dst_port;
}

int ext_key_hash(void* key)
{
  struct ext_key* ek = key; 
  return ek->dst_port;
}

// From double-map.c
struct DoubleMap {
  int value_size;

  uq_value_copy* cpy;
  uq_value_destr* dstr;

  uint8_t *values;

  int *bbs_a;
  void **kps_a;
  int *khs_a;
  int *chns_a;
  int *inds_a;
  map_keys_equality *eq_a;
  map_key_hash *hsh_a;

  int *bbs_b;
  void **kps_b;
  int *khs_b;
  int *chns_b;
  int *inds_b;
  map_keys_equality *eq_b;
  map_key_hash *hsh_b;

  dmap_extract_keys *exk;
  dmap_pack_keys *pk;

  int n_vals;
  int capacity;
  int keys_capacity;
};

// Render the arrangement of the flows in the flowtable.
void draw_flow_map() {
#ifdef LOG
  struct DoubleMap *flow_map = get_flow_table();
  for (int i = 0; i < flow_map->capacity; ++i) {
    printf("%d", flow_map->chns_b[i]);
  }
  printf("\n");
  for (int i = 0; i < flow_map->capacity; ++i) {
    if (flow_map->bbs_a[i]) {
      int hash = flow_map->khs_b[i];
      printf("%c", hash + '0');
    } else {
      printf(".");
    }
  }
  printf("\n");
#endif
}

void report_progress(int percent) {
  static int percent_reported = 0;
  if (percent_reported + REPORT_INCREMENT <= percent) {
    percent_reported += REPORT_INCREMENT;
    printf("[%d%%]\n", percent_reported);
    fflush(stdout);
  }
}

// From double-chain.c
struct DoubleChain {
  struct dchain_cell* cells;
  uint32_t *timestamps;
};

// From flowmanager.c
extern struct DoubleChain* chain;

// From double-chain-impl.c
enum DCHAIN_ENUM {
  ALLOC_LIST_HEAD = 0,
  FREE_LIST_HEAD = 1,
  INDEX_SHIFT = DCHAIN_RESERVED
};

void quick_setup(uint32_t *time) {

  allocate_flowmanager(2 /* # devices */,
                       1 /* starting port */,
                       0 /* external source ip */,
                       0 /* external device id */,
                       EXP_TIME /* expiration time */,
                       NUM_FLOWS /* max # flows */);

  struct DoubleMap *flow_map = get_flow_table();
  for (int i = 0; i < NUM_FLOWS; ++i) {
    int opposite_pos = (i - 1 + NUM_FLOWS)%NUM_FLOWS;
    struct flow flow = {
      .ik = {
        .int_src_port = 0,
        .dst_port = i,
        .int_src_ip = 0,
        .dst_ip = i,
        .int_device_id = 0,
        .protocol = 0
      },
      .ek = {
        .ext_src_port = 0,
        .dst_port = i,
        .ext_src_ip = 0,
        .dst_ip = i,
        .ext_device_id = 0,
        .protocol = 0
      },
      .int_src_port = 0,
      .ext_src_port = 0,
      .dst_port = i,
      .int_src_ip = 0,
      .ext_src_ip = 0,
      .dst_ip = i,
      .int_device_id = 0,
      .ext_device_id = 0,
      .protocol = 0
    };
    struct flow *dst_flow = &(((struct flow*)flow_map->values)[opposite_pos]);
    memcpy(dst_flow, &flow, sizeof(flow));
    flow_map->bbs_a[opposite_pos] = 1;
    flow_map->kps_a[opposite_pos] = &dst_flow->ik;
    flow_map->khs_a[opposite_pos] = int_key_hash(&dst_flow->ik);
    flow_map->chns_a[opposite_pos] = NUM_FLOWS - 1;
    flow_map->inds_a[opposite_pos] = opposite_pos;

    flow_map->bbs_b[opposite_pos] = 1;
    flow_map->kps_b[opposite_pos] = &dst_flow->ek;
    flow_map->khs_b[opposite_pos] = ext_key_hash(&dst_flow->ek);
    flow_map->chns_b[opposite_pos] = NUM_FLOWS - 1;
    flow_map->inds_b[opposite_pos] = opposite_pos;

  }
  flow_map->n_vals = NUM_FLOWS;
  for (int i = 0; i < NUM_FLOWS; ++i) {
    chain->timestamps[i] = 0;
    if (i == NUM_FLOWS - 1)
      chain->cells[i+INDEX_SHIFT].next = ALLOC_LIST_HEAD;
    else
      chain->cells[i+INDEX_SHIFT].next = i+INDEX_SHIFT+1;

    if (0 < i)
      chain->cells[i+INDEX_SHIFT].prev = i+INDEX_SHIFT-1;
    else
      chain->cells[i+INDEX_SHIFT].prev = ALLOC_LIST_HEAD;
  }
  chain->cells[ALLOC_LIST_HEAD].prev = INDEX_SHIFT + NUM_FLOWS - 1;
  chain->cells[ALLOC_LIST_HEAD].next = INDEX_SHIFT;
  chain->cells[FREE_LIST_HEAD].prev = FREE_LIST_HEAD;
  chain->cells[FREE_LIST_HEAD].next = FREE_LIST_HEAD;
  *time = 0;
}

void setup(uint32_t *time) {
  struct int_key ik = {0};
  struct int_key tip_flows[NUM_FLOWS];
  struct flow flow;
  *time = 0;

  allocate_flowmanager(2 /* # devices */,
                       1 /* starting port */,
                       0 /* external source ip */,
                       0 /* external device id */,
                       EXP_TIME /* expiration time */,
                       NUM_FLOWS /* max # flows */);

  printf("Preparing %d flows ...\n", NUM_FLOWS);
  fflush(stdout);

  draw_flow_map();

  for (int i = 0; i < NUM_FLOWS; ++i) {
    // Add i-th chain (NUM_FLOWS - i long) to the table
    for (int j = 0; j < NUM_FLOWS - i; ++j) {
      *time += 10;
      memset(&ik, 0, sizeof(ik));
      ik.dst_port = i;
      ik.dst_ip = *time;
      allocate_flow(&ik, *time, &flow);
      draw_flow_map();
    }
    tip_flows[i] = ik;
    // Rejuvenate previous tip flows to save them from expiration
    for (int k = 0; k < i; ++k) {
      get_flow_by_int_key(&tip_flows[k], *time, &flow);
    }
    // Expire all but tip flows
    expire_flows(*time + EXP_TIME - 1);
    report_progress(i*100/NUM_FLOWS);
    draw_flow_map();
  }
}

void measure(uint32_t time) {
  printf("Measurement...\n");
  fflush(stdout);

  struct timespec start_time, end_time;
  int result1 = clock_gettime(CLOCK_MONOTONIC, &start_time);

  // Expire all the flows left in the table
  expire_flows(time + EXP_TIME + 1);

  int result2 = clock_gettime(CLOCK_MONOTONIC, &end_time);
  if (result1 != 0 || result2 != 0) {
    printf("Problem with clock_gettime!\n");
    exit(-1);
  }
  printf("Time is %ld \n",
         ((end_time.tv_sec - start_time.tv_sec)*1000000000 +
          (end_time.tv_nsec - start_time.tv_nsec)));
}

int main(int argc, char *argv[]) {
  uint32_t time;

  //setup(&time);
  quick_setup(&time);

  draw_flow_map();
  printf("\n");

  measure(time);
}
