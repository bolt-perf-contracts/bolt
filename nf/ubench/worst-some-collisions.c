#include <string.h>
#include <stdio.h>
#include <time.h>
#include "x86intrin.h"

#include "lib/flowmanager.h"
#include "lib/flowtable.h"
#include "lib/containers/double-map.h"
#include "lib/containers/double-chain-impl.h"
#include "lib/containers/double-chain.h"

#define DO_EXPAND(VAL)  1 ## VAL
#define EXPAND(VAL)     DO_EXPAND(VAL)

#define NUM_FLOWS 65535
#define EXP_TIME 10
//#define LOG
#define REPORT_INCREMENT 5
#if !defined(NUM_COLLISIONS) || (EXPAND(NUM_COLLISIONS) == 1)
#  error "Must define NUM_COLLISIONS here."
#endif

#define max_size 41943040
long long* a;

void clear_cache(long time)
{
  long long sum=0;
  for(long long i=0;i<max_size;i++)
  { a[i] = i;
  }

   for(long long i=0;i<max_size;i++)
  { sum+=a[i];
  }
  if(time ==0) printf("Sum is %lld \n",sum);
}

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

// From flowmanager.c
struct FlowManager {
  uint16_t starting_port;
  uint32_t ext_src_ip;
  uint16_t ext_device_id;
  uint32_t expiration_time; /*seconds*/
  struct DoubleChain* chain;
  struct DoubleMap* flow_table;
};

// Render the arrangement of the flows in the flowtable.
void draw_flow_map(struct FlowManager* flow_manager) {
#ifdef LOG
  struct DoubleMap *flow_map = flow_manager.flow_table;
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
  struct dchain_cell *cells;
  uint32_t *timestamps;
};

// From double-chain-impl.c
enum DCHAIN_ENUM {
  ALLOC_LIST_HEAD = 0,
  FREE_LIST_HEAD = 1,
  INDEX_SHIFT = DCHAIN_RESERVED
};

struct FlowManager *quick_setup(uint32_t *time) {

  struct FlowManager *flow_manager =
    allocate_flowmanager(2 /* # devices */,
                         1 /* starting port */,
                         0 /* external source ip */,
                         0 /* external device id */,
                         EXP_TIME /* expiration time */,
                         NUM_FLOWS /* max # flows */);

  struct DoubleMap *flow_map = flow_manager->flow_table;
  struct DoubleChain *chain = flow_manager->chain;
  for (int i = 0; i < NUM_COLLISIONS + 1; ++i) {
    int pos = NUM_FLOWS - i - 1;
    struct flow flow = {
      .ik = {
        .int_src_port = 0,
        .dst_port = 0,
        .int_src_ip = 0,
        .dst_ip = i,
        .int_device_id = 0,
        .protocol = 0
      },
      .ek = {
        .ext_src_port = 0,
        .dst_port = 0,
        .ext_src_ip = 0,
        .dst_ip = i,
        .ext_device_id = 0,
        .protocol = 0
      },
      .int_src_port = 0,
      .ext_src_port = 0,
      .dst_port = 0,
      .int_src_ip = 0,
      .ext_src_ip = 0,
      .dst_ip = i,
      .int_device_id = 0,
      .ext_device_id = 0,
      .protocol = 0
    };
    struct flow *dst_flow = &(((struct flow*)flow_map->values)[pos]);
    memcpy(dst_flow, &flow, sizeof(flow));
    flow_map->bbs_a[pos] = 1;
    flow_map->kps_a[pos] = &dst_flow->ik;
    flow_map->khs_a[pos] = int_key_hash(&dst_flow->ik);
    flow_map->chns_a[pos] = NUM_FLOWS - NUM_COLLISIONS - 1 + i;
    flow_map->inds_a[pos] = pos;

    flow_map->bbs_b[pos] = 1;
    flow_map->kps_b[pos] = &dst_flow->ek;
    flow_map->khs_b[pos] = ext_key_hash(&dst_flow->ek);
    flow_map->chns_b[pos] = NUM_FLOWS - NUM_COLLISIONS - 1 + i;
    flow_map->inds_b[pos] = pos;
  }
  for (int i = 0; i < NUM_FLOWS - NUM_COLLISIONS - 1; ++i) {
    int flow_pos = i;
    int flow_id = i + 1;
    int opposite_hash = i;
    struct flow flow = {
      .ik = {
        .int_src_port = 0,
        .dst_port = flow_id,
        .int_src_ip = 0,
        .dst_ip = flow_id,
        .int_device_id = 0,
        .protocol = 0
      },
      .ek = {
        .ext_src_port = 0,
        .dst_port = flow_id,
        .ext_src_ip = 0,
        .dst_ip = flow_id,
        .ext_device_id = 0,
        .protocol = 0
      },
      .int_src_port = 0,
      .ext_src_port = 0,
      .dst_port = flow_id,
      .int_src_ip = 0,
      .ext_src_ip = 0,
      .dst_ip = flow_id,
      .int_device_id = 0,
      .ext_device_id = 0,
      .protocol = 0
    };
    struct flow *dst_flow = &(((struct flow*)flow_map->values)[flow_pos]);
    memcpy(dst_flow, &flow, sizeof(flow));
    flow_map->bbs_a[flow_pos] = 1;
    flow_map->kps_a[flow_pos] = &dst_flow->ik;
    flow_map->khs_a[flow_pos] = int_key_hash(&dst_flow->ik);
    flow_map->chns_a[flow_pos] = NUM_FLOWS - 1;
    flow_map->inds_a[flow_pos] = flow_pos;

    flow_map->bbs_b[flow_pos] = 1;
    flow_map->kps_b[flow_pos] = &dst_flow->ek;
    flow_map->khs_b[flow_pos] = ext_key_hash(&dst_flow->ek);
    flow_map->chns_b[flow_pos] = NUM_FLOWS - 1;
    flow_map->inds_b[flow_pos] = flow_pos;

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
  return flow_manager;
}

struct FlowManager *setup(uint32_t *time) {
  struct int_key ik = {0};
  struct int_key saved_flows[NUM_FLOWS];
  int n_saved_flows = 0;
  struct flow flow;
  *time = 0;

  struct FlowManager *flow_manager =
    allocate_flowmanager(2 /* # devices */,
                         1 /* starting port */,
                         0 /* external source ip */,
                         0 /* external device id */,
                         EXP_TIME /* expiration time */,
                         NUM_FLOWS /* max # flows */);

  printf("Preparing %d flows ...\n", NUM_FLOWS);
  fflush(stdout);

  draw_flow_map(flow_manager);

  for (int i = 0; i < NUM_FLOWS; ++i) {
    *time += 10;
    memset(&ik, 0, sizeof(ik));
    ik.dst_port = 0;
    ik.dst_ip = *time;
    allocate_flow(flow_manager, &ik, *time, &flow);
    draw_flow_map(flow_manager);
    if (NUM_FLOWS - NUM_COLLISIONS - 1 <= i) {
      saved_flows[n_saved_flows] = ik;
      ++n_saved_flows;
    }
  }
  for (int k = 0; k < n_saved_flows; ++k) {
    get_flow_by_int_key(flow_manager, &saved_flows[k], *time, &flow);
  }
  expire_flows(flow_manager, *time + EXP_TIME - 1);
  draw_flow_map(flow_manager);

  for (int chain_id = 1; n_saved_flows < NUM_FLOWS; ++chain_id) {
    // Add i-th chain (NUM_FLOWS - i long) to the table
    for (int j = 0; j < NUM_FLOWS - n_saved_flows; ++j) {
      *time += 10;
      memset(&ik, 0, sizeof(ik));
      ik.dst_port = chain_id;
      ik.dst_ip = *time;
      allocate_flow(flow_manager, &ik, *time, &flow);
      draw_flow_map(flow_manager);
    }
    saved_flows[n_saved_flows] = ik;
    ++n_saved_flows;
    // Rejuvenate previous tip flows to save them from expiration
    for (int k = 0; k < n_saved_flows; ++k) {
      get_flow_by_int_key(flow_manager, &saved_flows[k], *time, &flow);
    }
    // Expire all but tip flows
    expire_flows(flow_manager, *time + EXP_TIME - 1);
    draw_flow_map(flow_manager);
    //report_progress(i*100/NUM_FLOWS);
  }
  return flow_manager;
}

void measure(uint32_t time, struct FlowManager *flow_manager) {
  //printf("Measurement...\n");
  fflush(stdout);

  struct timespec start_time, end_time;
  int result1 = clock_gettime(CLOCK_MONOTONIC, &start_time);

  // Expire all the flows left in the table
  expire_flows(flow_manager, time + EXP_TIME + 1);

  int result2 = clock_gettime(CLOCK_MONOTONIC, &end_time);
  if (result1 != 0 || result2 != 0) {
    printf("Problem with clock_gettime!\n");
    exit(-1);
  }
  printf("%ld \n",
         ((end_time.tv_sec - start_time.tv_sec)*1000000000 +
          (end_time.tv_nsec - start_time.tv_nsec)));
}

int main(int argc, char *argv[]) {
  uint32_t time;
  a = malloc(sizeof(*a)*max_size);
  if(a==NULL) printf("malloc_failed\n");
   struct timespec gNeverZero;
   int ret = clock_gettime(CLOCK_MONOTONIC,&gNeverZero);

  //setup(&time);
  struct FlowManager * flow_manager = quick_setup(&time);

  draw_flow_map(flow_manager);
  clear_cache(gNeverZero.tv_nsec);
  measure(time, flow_manager);
}
