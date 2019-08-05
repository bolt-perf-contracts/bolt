#ifdef KLEE_VERIFICATION
#include <klee/klee.h>
#include "lib/stubs/containers/map-stub-control.h"
#include "lib/stubs/containers/double-chain-stub-control.h"
#include "lib/stubs/containers/vector-stub-control.h"
#include "bridge_loop.h"
#endif //KLEE_VERIFICATION

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
// DPDK uses these but doesn't include them. :|
#include <linux/limits.h>
#include <sys/types.h>
#include <unistd.h>
#include <rte_common.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <cmdline_parse_etheraddr.h>

#include "lib/nf_forward.h"
#include "lib/nf_util.h"
#include "lib/nf_log.h"
#include "bridge_config.h"
#include "bridge_data.h"

#include "lib/containers/double-chain.h"
#include "lib/containers/map.h"
#include "lib/containers/vector.h"
#include "lib/expirator.h"

struct bridge_config config;

struct StaticFilterTable static_ft;
struct DynamicFilterTable dynamic_ft;

enum TrafficClass NF_TRAFFIC_CLASS = UNDEFINED;

int bridge_expire_entries(time_t time)
{
  if (time < config.expiration_time)
    return 0;

  // This is convoluted - we want to make sure the sanitization doesn't
  // extend our time_t value in 128 bits, which would confuse the validator.
  // So we "prove" by hand that it's OK...
  // We know time >= 0 since time >= config.expiration_time
  assert(sizeof(time_t) <= sizeof(uint64_t));
  uint64_t time_u = (uint64_t)time;                      // OK since assert above passed and time > 0
  uint64_t min_time_u = time_u - config.expiration_time; // OK because time >= expiration_time >= 0
  assert(sizeof(uint64_t) <= sizeof(time_t));
  time_t min_time = (time_t)min_time_u; // OK since the assert above passed

  return expire_items_single_map(dynamic_ft.heap, dynamic_ft.keys,
                                 dynamic_ft.map,
                                 min_time);
}

int bridge_get_device(struct ether_addr *dst,
                      uint16_t src_device)
{
  int device = -1;
  struct StaticKey k;
  memcpy(&k.addr, dst, sizeof(struct ether_addr));
  k.device = src_device;
  int hash = static_key_hash(&k);
#ifdef NO_STATIC_MAPPING
  int present = 0;
#else  //NO_STATIC_MAPPING
  int present = map_get(static_ft.map,
                        &k, &device);
#endif //NO_STATIC_MAPPING
  if (present)
  {
    return device;
  }

  int index = -1;
  hash = ether_addr_hash(dst);
  present = map_get(dynamic_ft.map, dst, &index);
  if (present)
  {
#ifdef DUMP_PERF_VARS
    printf("Unicast\n");
#endif
    VIGOR_TAG(TRAFFIC_CLASS, KNOWN_DEST);
    struct DynamicValue *value = 0;
    vector_borrow_full(dynamic_ft.values, index, (void **)&value);
    device = value->device;
    vector_return_full(dynamic_ft.values, index, value);
    return device;
  }
  VIGOR_TAG(TRAFFIC_CLASS, UNKNOWN_DEST);
  return -1;
}

void bridge_put_update_entry(struct ether_addr *src,
                             uint16_t src_device,
                             time_t time)
{
  int index = -1;
  int hash = ether_addr_hash(src);
  int present = map_get(dynamic_ft.map, src, &index);
  if (present)
  {
    dchain_rejuvenate_index(dynamic_ft.heap, index, time);
    VIGOR_TAG(TRAFFIC_CLASS, KNOWN_SRC);
  }
  else
  {
    int allocated = dchain_allocate_new_index(dynamic_ft.heap,
                                              &index,
                                              time);
    if (!allocated)
    {
      NF_INFO("No more space in the dynamic table");
      VIGOR_TAG(TRAFFIC_CLASS, UNKNOWN_SRC_FULL);
      return;
    }
    struct ether_addr *key = 0;
    struct DynamicValue *value = 0;
    vector_borrow_full(dynamic_ft.keys, index, (void **)&key);
    vector_borrow_full(dynamic_ft.values, index, (void **)&value);
    memcpy(key, src, sizeof(struct ether_addr));
    value->device = src_device;
    map_put(dynamic_ft.map, key, index);
    VIGOR_TAG(TRAFFIC_CLASS, UNKNOWN_SRC);
    //the other half of the key is in the map
    vector_return_half(dynamic_ft.keys, index, key);
    vector_return_full(dynamic_ft.values, index, value);
  }
}

void allocate_static_ft(int capacity)
{
  assert(0 < capacity);
  assert(capacity < CAPACITY_UPPER_LIMIT);
  int happy = map_allocate(static_key_eq, static_key_hash,
                           capacity, &static_ft.map);

  if (!happy)
    rte_exit(EXIT_FAILURE, "error allocating static map");
  happy = vector_allocate(sizeof(struct StaticKey), capacity,
                          init_nothing_st,
                          &static_ft.keys);
  if (!happy)
    rte_exit(EXIT_FAILURE, "error allocating static array");
}
#ifdef KLEE_VERIFICATION

struct str_field_descr dynamic_vector_key_fields[] = {
    {0, sizeof(uint8_t), "a"},
    {1, sizeof(uint8_t), "b"},
    {2, sizeof(uint8_t), "c"},
    {3, sizeof(uint8_t), "d"},
    {4, sizeof(uint8_t), "e"},
    {5, sizeof(uint8_t), "f"},
};

struct str_field_descr dynamic_vector_value_fields[] = {
    {0, sizeof(uint16_t), "device"},
};

int stat_map_condition(void *key, int index)
{
  // Do not trace the model service function
  return 0 <= index & index < rte_eth_dev_count();
}

#endif //KLEE_VERIFICATION

#ifdef KLEE_VERIFICATION
#ifdef NO_STATIC_MAPPING
void read_static_ft_from_file()
{
  static_ft.map = NULL;
  static_ft.keys = NULL;
}
#else  //NO_STATIC_MAPPING
//TODO: this function must appear in the traces.
// let's see if we notice that it does not
void read_static_ft_from_file()
{

  int static_capacity = klee_range(1, CAPACITY_UPPER_LIMIT, "static_capacity");
  allocate_static_ft(static_capacity);
  map_set_key_size(static_ft.map, sizeof(struct StaticKey));

  map_set_entry_condition(static_ft.map, stat_map_condition);
  vector_set_layout(static_ft.keys, static_map_key_fields,
                    sizeof(static_map_key_fields) /
                        sizeof(static_map_key_fields[0]),
                    static_map_key_nested_fields,
                    sizeof(static_map_key_nested_fields) /
                        sizeof(static_map_key_nested_fields[0]),
                    "StaticKey");
}
#endif //NO_STATIC_MAPPING

#else //KLEE_VERIFICATION

void read_static_ft_from_file()
{
  if (config.static_config_fname[0] == '\0')
  {
    // No static config
    allocate_static_ft(1);
    return;
  }

  FILE *cfg_file = fopen(config.static_config_fname, "r");
  if (cfg_file == NULL)
  {
    rte_exit(EXIT_FAILURE, "Error opening the static config file: %s",
             config.static_config_fname);
  }

  int number_of_lines = 0;
  char ch;
  do
  {
    ch = fgetc(cfg_file);
    if (ch == '\n')
      number_of_lines++;
  } while (ch != EOF);

  // Make sure the hash table is occupied only by 50%
  int capacity = number_of_lines * 2;
  rewind(cfg_file);
  if (CAPACITY_UPPER_LIMIT <= capacity)
  {
    rte_exit(EXIT_FAILURE, "Too many static rules (%d), max: %d",
             number_of_lines, CAPACITY_UPPER_LIMIT / 2);
  }
  allocate_static_ft(capacity);
  int count = 0;

  while (1)
  {
    char mac_addr_str[20];
    char source_str[10];
    char target_str[10];
    int result = fscanf(cfg_file, "%18s", mac_addr_str);
    if (result != 1)
    {
      if (result == EOF)
        break;
      else
      {
        NF_INFO("Cannot read MAC address from file: %s",
                strerror(errno));
        goto finally;
      }
    }

    result = fscanf(cfg_file, "%9s", source_str);
    if (result != 1)
    {
      if (result == EOF)
      {
        NF_INFO("Incomplete config string: %s, skip", mac_addr_str);
        break;
      }
      else
      {
        NF_INFO("Cannot read the filtering target for MAC %s: %s",
                mac_addr_str, strerror(errno));
        goto finally;
      }
    }

    result = fscanf(cfg_file, "%9s", target_str);
    if (result != 1)
    {
      if (result == EOF)
      {
        NF_INFO("Incomplete config string: %s, skip", mac_addr_str);
        break;
      }
      else
      {
        NF_INFO("Cannot read the filtering target for MAC %s: %s",
                mac_addr_str, strerror(errno));
        goto finally;
      }
    }

    int device_from;
    int device_to;
    char *temp;
    struct StaticKey *key = 0;
    vector_borrow_full(static_ft.keys, count, (void **)&key);

    // Ouff... the strings are extracted, now let's parse them.
    result = cmdline_parse_etheraddr(NULL, mac_addr_str,
                                     &key->addr,
                                     sizeof(struct ether_addr));
    if (result < 0)
    {
      NF_INFO("Invalid MAC address: %s, skip",
              mac_addr_str);
      continue;
    }

    device_from = strtol(source_str, &temp, 10);
    if (temp == source_str || *temp != '\0')
    {
      NF_INFO("Non-integer value for the forwarding rule: %s (%s), skip",
              mac_addr_str, target_str);
      continue;
    }

    device_to = strtol(target_str, &temp, 10);
    if (temp == target_str || *temp != '\0')
    {
      NF_INFO("Non-integer value for the forwarding rule: %s (%s), skip",
              mac_addr_str, target_str);
      continue;
    }

    // Now everything is alright, we can add the entry
    key->device = device_from;
    map_put(static_ft.map, &key->addr, device_to);
    vector_return_half(static_ft.keys, count, key);
    ++count;
    assert(count < capacity);
  }
finally:
  fclose(cfg_file);
}

#endif //KLEE_VERIFICATION

void nf_core_init(void)
{
#ifndef NO_STATIC_MAPPING
  read_static_ft_from_file();
#endif

  int capacity = config.dyn_capacity;
  int happy = map_allocate(ether_addr_eq, ether_addr_hash,
                           capacity, &dynamic_ft.map);
  if (!happy)
    rte_exit(EXIT_FAILURE, "error allocating dynamic map");
  happy = vector_allocate(sizeof(struct ether_addr), capacity,
                          init_nothing_ea,
                          &dynamic_ft.keys);
  if (!happy)
    rte_exit(EXIT_FAILURE, "error allocating dynamic key array");
  happy = vector_allocate(sizeof(struct DynamicValue), capacity,
                          init_nothing_dv,
                          &dynamic_ft.values);
  if (!happy)
    rte_exit(EXIT_FAILURE, "error allocating dynamic value array");
  happy = dchain_allocate(capacity, &dynamic_ft.heap);
  if (!happy)
    rte_exit(EXIT_FAILURE, "error allocating heap");

#ifdef KLEE_VERIFICATION
  map_set_key_size(dynamic_ft.map, sizeof(struct ether_addr));
  vector_set_layout(dynamic_ft.keys,
                    dynamic_vector_key_fields,
                    sizeof(dynamic_vector_key_fields) /
                        sizeof(dynamic_vector_key_fields[0]),
                    NULL, 0,
                    "ether_addr");
  vector_set_layout(dynamic_ft.values,
                    dynamic_vector_value_fields,
                    sizeof(dynamic_vector_value_fields) /
                        sizeof(dynamic_vector_value_fields[0]),
                    NULL, 0,
                    "DynamicValue");
#endif //KLEE_VERIFICATION
}

int nf_core_process(struct rte_mbuf *mbuf,
                    time_t now)
{
  struct ether_hdr *ether_header = nf_get_mbuf_ether_header(mbuf);

  bridge_expire_entries(now);
  bridge_put_update_entry(&ether_header->s_addr, mbuf->port, now);

  int dst_device = bridge_get_device(&ether_header->d_addr,
                                     mbuf->port);

  if (dst_device == -1)
  {
    return FLOOD_FRAME;
  }

  if (dst_device == -2)
  {
    NF_DEBUG("filtered frame");
    return mbuf->port;
  }

#ifdef KLEE_VERIFICATION
  // HACK concretize it - need to fix/move this
  klee_assume(dst_device < rte_eth_dev_count());
  for (unsigned d = 0; d < rte_eth_dev_count(); d++)
    if (dst_device == d)
    {
      dst_device = d;
      break;
    }
#endif

  return dst_device;
}

void nf_config_init(int argc, char **argv)
{
  bridge_config_init(&config, argc, argv);
}

void nf_config_cmdline_print_usage(void)
{
  bridge_config_cmdline_print_usage();
}

void nf_print_config()
{
  bridge_print_config(&config);
}

#ifdef KLEE_VERIFICATION

void nf_loop_iteration_begin(unsigned lcore_id,
                             time_t time)
{
  bridge_loop_iteration_begin(&dynamic_ft.heap,
                              &dynamic_ft.map,
                              &dynamic_ft.keys,
                              &dynamic_ft.values,
                              &static_ft.map,
                              &static_ft.keys,
                              config.dyn_capacity,
                              time,
                              rte_eth_dev_count());
}

void nf_add_loop_iteration_assumptions(unsigned lcore_id,
                                       time_t time)
{
  bridge_loop_iteration_assumptions(&dynamic_ft.heap,
                                    &dynamic_ft.map,
                                    &dynamic_ft.keys,
                                    &dynamic_ft.values,
                                    &static_ft.map,
                                    &static_ft.keys,
                                    config.dyn_capacity,
                                    time);
}

void nf_loop_iteration_end(unsigned lcore_id,
                           time_t time)
{
  bridge_loop_iteration_end(&dynamic_ft.heap,
                            &dynamic_ft.map,
                            &dynamic_ft.keys,
                            &dynamic_ft.values,
                            &static_ft.map,
                            &static_ft.keys,
                            config.dyn_capacity,
                            time,
                            rte_eth_dev_count());
}

#endif //KLEE_VERIFICATION
