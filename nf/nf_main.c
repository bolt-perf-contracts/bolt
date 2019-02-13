#ifdef KLEE_VERIFICATION
#include "lib/stubs/time_stub_control.h"
#include "lib/stubs/driver_stub.h"
#include "lib/stubs/hardware_stub.h"
#include <klee/klee.h>
#endif

#include <inttypes.h>

//#define REPLAY

// TX configuration default values 
#define IXGBE_DEFAULT_TX_FREE_THRESH  32
#define IXGBE_DEFAULT_TX_PTHRESH      32
#define IXGBE_DEFAULT_TX_HTHRESH      0
#define IXGBE_DEFAULT_TX_WTHRESH      0
#define IXGBE_DEFAULT_TX_RSBIT_THRESH 32

#ifdef KLEE_VERIFICATION
#  define VIGOR_LOOP_BEGIN \
    unsigned _vigor_lcore_id = rte_lcore_id(); \
    time_t _vigor_start_time = start_time(); \
    int _vigor_loop_termination = klee_int("loop_termination"); \
    unsigned VIGOR_DEVICES_COUNT;                                       \
    klee_possibly_havoc(&VIGOR_DEVICES_COUNT, sizeof(VIGOR_DEVICES_COUNT), "VIGOR_DEVICES_COUNT"); \
    time_t VIGOR_NOW;                                                   \
    klee_possibly_havoc(&VIGOR_NOW, sizeof(VIGOR_NOW), "VIGOR_NOW");    \
    unsigned VIGOR_DEVICE;                                              \
    klee_possibly_havoc(&VIGOR_DEVICE, sizeof(VIGOR_DEVICE), "VIGOR_DEVICE"); \
    unsigned _d;                                                        \
    klee_possibly_havoc(&_d, sizeof(_d), "_d");                         \
    while(klee_induce_invariants() & _vigor_loop_termination) { \
      nf_add_loop_iteration_assumptions(_vigor_lcore_id, _vigor_start_time); \
      nf_loop_iteration_begin(_vigor_lcore_id, _vigor_start_time);      \
      VIGOR_NOW = current_time(); \
      /* concretize the device to avoid leaking symbols into DPDK */ \
      VIGOR_DEVICES_COUNT = rte_eth_dev_count(); \
      VIGOR_DEVICE = klee_range(0, VIGOR_DEVICES_COUNT, "VIGOR_DEVICE"); \
      for(_d = 0; _d < VIGOR_DEVICES_COUNT; _d++) if (VIGOR_DEVICE == _d) { VIGOR_DEVICE = _d; break; } \
      stub_hardware_receive_packet(VIGOR_DEVICE);
#  ifdef REPLAY
#    define VIGOR_LOOP_END                                     \
        stub_hardware_reset_receive(VIGOR_DEVICE);             \
        nf_loop_iteration_end(_vigor_lcore_id, VIGOR_NOW);     \
        if (klee_int("tired")) {                               \
          exit(0);                                             \
        }                                                      \
      }
#  else//REPLAY
#    define VIGOR_LOOP_END                                \
      stub_hardware_reset_receive(VIGOR_DEVICE);          \
      nf_loop_iteration_end(_vigor_lcore_id, VIGOR_NOW);  \
      }
#  endif//REPLAY
#else//KLEE_VERIFICATION
#  define VIGOR_LOOP_BEGIN \
    while (1) { \
      time_t VIGOR_NOW = current_time(); \
      unsigned VIGOR_DEVICES_COUNT = rte_eth_dev_count(); \
      for (uint16_t VIGOR_DEVICE = 0; VIGOR_DEVICE < VIGOR_DEVICES_COUNT; VIGOR_DEVICE++) {
#  define VIGOR_LOOP_END } }
#endif//KLEE_VERIFICATION


// DPDK uses these but doesn't include them. :|
#include <linux/limits.h>
#include <sys/types.h>

#include <stdlib.h>
#include <string.h>

#include <rte_common.h>
#include <rte_eal.h>
#include <rte_errno.h>
#include <rte_ethdev.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>

#include "lib/nf_forward.h"
#include "lib/nf_log.h"
#include "lib/nf_time.h"
#include "lib/nf_util.h"


// Number of RX/TX queues
static const uint16_t RX_QUEUES_COUNT = 1;
static const uint16_t TX_QUEUES_COUNT = 1;


#ifdef DUMP_PERF
#include "lib/nf_log.h"
#include <time.h>
#include "x86intrin.h"
struct timespec start_time1, end_time;
#define batch_param 10000
struct procTimeEntry{
  int traffic_class;
  long processing_time;
} processing_times[batch_param];
long ctr = 0;

extern int TRAFFIC_CLASS;
#endif//DUMP_PERF

// --- Static config ---
// TODO see remark in lcore_main
// Size of batches to receive; trade-off between latency and throughput
// Can be overriden at compile time
//#ifndef BATCH_SIZE
//static const uint16_t BATCH_SIZE = 32;
//#endif

// Queue sizes for receiving/transmitting packets
// NOT powers of 2 so that ixgbe doesn't use vector stuff
// but they have to be multiples of 8, and at least 32, otherwise the driver refuses
static const uint16_t RX_QUEUE_SIZE = 96;
static const uint16_t TX_QUEUE_SIZE = 96;

// Clone pool for flood()
static struct rte_mempool* clone_pool;

// Buffer count for mempools
static const unsigned MEMPOOL_BUFFER_COUNT = 256;


// --- Initialization ---
static int
nf_init_device(uint16_t device, struct rte_mempool *mbuf_pool)
{
  int retval;

  // device_conf passed to rte_eth_dev_configure cannot be NULL
  struct rte_eth_conf device_conf;
  memset(&device_conf, 0, sizeof(struct rte_eth_conf));

  // Configure the device
  retval = rte_eth_dev_configure(
    device,
    RX_QUEUES_COUNT,
    TX_QUEUES_COUNT,
    &device_conf
  );
  if (retval != 0) {
    return retval;
  }

  struct rte_eth_txconf *tx_confp = NULL;
  //This config breaks verify-hardware. however it is nessary to ensure,
  //we avoid batch buffer recycling on some packets. It enforces the dpdk to
  //free a buffer right after a packet is sent (after some warm-up period,
  //like starting from a packet#95).
  //In verify-klee, this config is ignored anyway.
  //In verify-hardware, this config apprently leads to an explosion. Instead,
  //to ensure the state at the end of an iteration (send+receive) matches
  //the one at the beginning, we employ a "hack" -stub_hardware_reset_receive
#ifndef KLEE_VERIFICATION
  // Allocate and set up TX queues
  struct rte_eth_txconf tx_conf;
  tx_conf = (struct rte_eth_txconf) {
                .tx_thresh = {
                        .pthresh = IXGBE_DEFAULT_TX_PTHRESH,
                        .hthresh = IXGBE_DEFAULT_TX_HTHRESH,
                        .wthresh = IXGBE_DEFAULT_TX_WTHRESH,
                },
                .tx_free_thresh = IXGBE_DEFAULT_TX_FREE_THRESH,
                .tx_rs_thresh = IXGBE_DEFAULT_TX_RSBIT_THRESH,
                .txq_flags = ETH_TXQ_FLAGS_NOMULTSEGS |
                                ETH_TXQ_FLAGS_NOOFFLOADS,
        };
  
  tx_conf.tx_free_thresh = 1;
  tx_conf.tx_rs_thresh = 1;
  tx_confp = &tx_conf;
#endif//!KLEE_VERIFICATION
 
  for (int txq = 0; txq < TX_QUEUES_COUNT; txq++) {
    retval = rte_eth_tx_queue_setup(
      device,
      txq,
      TX_QUEUE_SIZE,
      rte_eth_dev_socket_id(device),
      tx_confp
	);
    if (retval != 0) {
      return retval;
    }
  }

  // Allocate and set up RX queues
  // with rx_free_thresh = 1 so that internal descriptors are replenished always,
  // i.e. 1 mbuf is taken (for RX) from the pool and 1 is put back (when freeing),
  //      at each iteration, which avoids havocing problems
  struct rte_eth_rxconf rx_conf;
  memset(&rx_conf, 0, sizeof(struct rte_eth_rxconf));
  rx_conf.rx_free_thresh = 1;
  for (int rxq = 0; rxq < RX_QUEUES_COUNT; rxq++) {
    retval = rte_eth_rx_queue_setup(
      device,
      rxq,
      RX_QUEUE_SIZE,
      rte_eth_dev_socket_id(device),
      &rx_conf,
      mbuf_pool
    );
    if (retval != 0) {
      return retval;
    }
  }

  // Start the device
  retval = rte_eth_dev_start(device);
  if (retval != 0) {
    return retval;
  }

  // Enable RX in promiscuous mode, just in case
  rte_eth_promiscuous_enable(device);
  if (rte_eth_promiscuous_get(device) != 1) {
    return retval;
  }

  return 0;
}


// Flood method for the bridge
#ifdef KLEE_VERIFICATION
void flood(struct rte_mbuf* frame, uint16_t skip_device, uint16_t nb_devices); // defined in stubs
#else
void
flood(struct rte_mbuf* frame, uint16_t skip_device, uint16_t nb_devices) {
  for (uint16_t device = 0; device < nb_devices; device++) {
    if (device == skip_device) continue;
    struct rte_mbuf* copy = rte_pktmbuf_clone(frame, clone_pool);
    if (copy == NULL) {
      rte_exit(EXIT_FAILURE, "Cannot clone a frame for flooding");
    }
    uint16_t actual_tx_len = rte_eth_tx_burst(device, 0, &copy, 1);

    if (actual_tx_len == 0) {
      rte_pktmbuf_free(copy);
    }
  }
  rte_pktmbuf_free(frame);
}
#endif//!KLEE_VERIFICATION


// --- Per-core work ---

static void
lcore_main(void)
{
  // TODO is this check useful?
  for (uint16_t device = 0; device < rte_eth_dev_count(); device++) {
    if (rte_eth_dev_socket_id(device) > 0 && rte_eth_dev_socket_id(device) != (int) rte_socket_id()) {
      NF_INFO("Device %" PRIu8 " is on remote NUMA node to polling thread.", device);
    }
  }

  nf_core_init();

  NF_INFO("Core %u forwarding packets.", rte_lcore_id());

  uint16_t actual_rx_len;
  uint16_t dst_device;
  struct rte_mbuf* buf;
  uint16_t actual_tx_len;
#ifdef KLEE_VERIFICATION
  klee_possibly_havoc(&actual_rx_len, sizeof(actual_rx_len), "actual_rx_len");
  klee_possibly_havoc(&dst_device, sizeof(dst_device), "dst_device");
  klee_possibly_havoc(&buf, sizeof(buf), "buf_addr");
  klee_possibly_havoc(&actual_tx_len, sizeof(actual_tx_len), "actual_tx_len");
#endif//KLEE_VERIFICATION
  VIGOR_LOOP_BEGIN
    buf = NULL;

#ifdef DUMP_PERF
      int gettime_result1 = clock_gettime(CLOCK_MONOTONIC, &start_time1);
#endif//DUMP_PERF

    actual_rx_len = rte_eth_rx_burst(VIGOR_DEVICE, 0, &buf, 1);

    if (actual_rx_len != 0) {

#ifdef DUMP_PERF_VARS
    NF_PERF_DEBUG("lcore_main:New_packet");
#endif//DUMP_PERF_VARS

      dst_device = nf_core_process(&buf[0], VIGOR_NOW);

      if (dst_device == VIGOR_DEVICE) {
        rte_pktmbuf_free(buf);
      } else if (dst_device == FLOOD_FRAME) {
        flood(buf, VIGOR_DEVICE, VIGOR_DEVICES_COUNT);
#ifdef DUMP_PERF_VARS
    printf("Broadcast\n");
#endif 
      } else {
        actual_tx_len = rte_eth_tx_burst(dst_device, 0, &buf, 1);
        if (actual_tx_len == 0) {
          rte_pktmbuf_free(buf);
        }
      }
    }
#ifdef DUMP_PERF
      int gettime_result2 = clock_gettime(CLOCK_MONOTONIC, &end_time);
      if (gettime_result1 == 0 && gettime_result2 == 0 && actual_rx_len!=0) {
        processing_times[ctr].processing_time =
             ((end_time.tv_sec - start_time1.tv_sec)*1000000000 +
              (end_time.tv_nsec - start_time1.tv_nsec));
        ctr++;
        if(batch_param <= ctr) {
          for(int i = 0; i < batch_param; i++) {
            printf("Time for class %d is %ld\n",
                   processing_times[i].traffic_class,
                   processing_times[i].processing_time);
          }
          fflush(stdout);
          ctr = 0;
          fflush(stdout);
        }
      }
#endif//DUMP_PERF

#ifdef STOP_ON_RX_0
    else if(VIGOR_DEVICE == 0) { return; }
#endif
#ifdef STOP_ON_RX_1
    else if(VIGOR_DEVICE == 1) { return; }
#endif
  VIGOR_LOOP_END
}


// --- Main ---

int
main(int argc, char* argv[])
{
  // Initialize the Environment Abstraction Layer (EAL)
  int ret = rte_eal_init(argc, argv);
  if (ret < 0) {
    rte_exit(EXIT_FAILURE, "Error with EAL initialization, ret=%d\n", ret);
  }
  argc -= ret;
  argv += ret;

#ifdef KLEE_VERIFICATION
  // Attach stub driver (note that hardware stub is autodetected, no need to attach)
  stub_driver_attach();
#endif

  // NF-specific config
  nf_config_init(argc, argv);
  nf_print_config();

  // Create a memory pool
  unsigned nb_devices = rte_eth_dev_count();
  struct rte_mempool* mbuf_pool = rte_pktmbuf_pool_create(
    "MEMPOOL", // name
    MEMPOOL_BUFFER_COUNT * nb_devices, // #elements
    0, // cache size (per-lcore, not useful in a single-threaded app)
    0, // application private area size
    RTE_MBUF_DEFAULT_BUF_SIZE, // data buffer size
    rte_socket_id() // socket ID
  );
  if (mbuf_pool == NULL) {
    rte_exit(EXIT_FAILURE, "Cannot create mbuf pool: %s\n", rte_strerror(rte_errno));
  }

  // Create another pool for the flood() cloning
  clone_pool = rte_pktmbuf_pool_create(
    "clone_pool", // name
     MEMPOOL_BUFFER_COUNT, // #elements
     0, // cache size (same remark as above)
     0, // application private data size
     RTE_MBUF_DEFAULT_BUF_SIZE, // data buffer size
     rte_socket_id() // socket ID
  );
  if (clone_pool == NULL) {
    rte_exit(EXIT_FAILURE, "Cannot create mbuf clone pool: %s\n", rte_strerror(rte_errno));
  }

  // Initialize all devices
  for (uint16_t device = 0; device < nb_devices; device++) {
    ret = nf_init_device(device, mbuf_pool);
    if (ret == 0) {
      NF_INFO("Initialized device %" PRIu8 ".", device);
    } else {
      rte_exit(EXIT_FAILURE, "Cannot init device %" PRIu8 ", ret=%d", device, ret);
    }
  }

  // Run!
  // ...in single-threaded mode, that is.
  lcore_main();

  return 0;
}
