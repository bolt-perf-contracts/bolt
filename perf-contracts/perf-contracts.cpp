#include "klee/perf-contracts.h"
#include <algorithm>
#include <iostream>
#include <iterator>
#include <assert.h>
#include <vector>
typedef long (*perf_calc_fn_ptr)(std::string, std::vector<long>);
typedef std::map<std::string, std::set<int> >(*cstate_fn_ptr)(
    std::vector<long>);

std::vector<std::string> supported_metrics;
std::vector<std::string> fn_names;
std::map<std::string, std::string> user_variables;
std::map<std::string, std::set<std::string> > optimization_variables;
std::map<std::string, std::map<int, std::string> > constraints;
std::map<std::string, std::map<int, perf_calc_fn_ptr> > perf_fn_ptrs;
std::map<std::string, std::map<int, cstate_fn_ptr> > cstate_fn_ptrs;

std::map<std::string, std::vector<std::string> > fn_user_variables;
std::map<std::string, std::vector<std::string> > fn_optimization_variables;
std::map<std::string, std::vector<std::string> > fn_variables;
std::vector<std::string> relevant_vars;
std::vector<long> relevant_vals;
/* Functions to calculate performance */

#define DRAM_LATENCY 200
#define L1_LATENCY 2
#define REHASHING_THRESHOLD 100
/*Helper Functions */

long ether_addr_hash_contract(std::string metric) {
  long constant;
  if (metric == "instruction count")
    constant = 3;
  else if (metric == "memory instructions")
    constant = 3;
  else if (metric == "execution cycles")
    constant = 0 * DRAM_LATENCY + 3 * L1_LATENCY + 0;
  return constant;
}

long ether_addr_eq_contract(std::string metric, long success) {
  long constant;
  if (metric == "instruction count")
    constant = 31;
  else if (metric == "memory instructions")
    constant = 9;
  else if (metric == "execution cycles")
    constant = 2 * DRAM_LATENCY + 7 * L1_LATENCY + 0;
  return constant;
}
long int_key_eq_contract(std::string metric, long success) {
  long constant;
  if (metric == "instruction count")
    constant = 20;
  else if (metric == "memory instructions")
    constant = 12;
  else if (metric == "execution cycles")
    constant = 1 * DRAM_LATENCY + 11 * L1_LATENCY + 17;
  return constant;
}

long ext_key_eq_contract(std::string metric, long success) {
  long constant;
  if (metric == "instruction count")
    constant = 20;
  else if (metric == "memory instructions")
    constant = 12;
  else if (metric == "execution cycles")
    constant = 1 * DRAM_LATENCY + 11 * L1_LATENCY + 17;
  return constant;
}

long int_key_hash_contract(std::string metric) {
  long constant;
  if (metric == "instruction count")
    constant = 40;
  else if (metric == "memory instructions")
    constant = 8;
  else if (metric == "execution cycles")
    constant = 0 * DRAM_LATENCY + 8 * L1_LATENCY + 55;
  return constant;
}

long ext_key_hash_contract(std::string metric) {
  long constant;
  if (metric == "instruction count")
    constant = 40;
  else if (metric == "memory instructions")
    constant = 8;
  else if (metric == "execution cycles")
    constant = 0 * DRAM_LATENCY + 8 * L1_LATENCY + 55;
  return constant;
}

long flow_cpy_contract(std::string metric) {
  long constant;
  if (metric == "instruction count")
    constant = 43;
  else if (metric == "memory instructions")
    constant = 44;
  else if (metric == "execution cycles")
    constant = 2 * DRAM_LATENCY + 42 * L1_LATENCY + 1;
  return constant;
}

long flow_destroy_contract(std::string metric) {
  long constant;
  if (metric == "instruction count")
    constant = 1;
  else if (metric == "memory instructions")
    constant = 2;
  else if (metric == "execution cycles")
    constant = 0 * DRAM_LATENCY + 2 * L1_LATENCY + 1;
  return constant;
}

long flow_extract_keys_contract(std::string metric) {
  long constant;
  if (metric == "instruction count")
    constant = 4;
  else if (metric == "memory instructions")
    constant = 4;
  else if (metric == "execution cycles")
    constant = 0 * DRAM_LATENCY + 4 * L1_LATENCY + 2;
  return constant;
}

long flow_pack_keys_contract(std::string metric) {
  long constant;
  if (metric == "instruction count")
    constant = 1;
  else if (metric == "memory instructions")
    constant = 2;
  else if (metric == "execution cycles")
    constant = 0 * DRAM_LATENCY + 2 * L1_LATENCY + 1;
  return constant;
}
/* Hack for freeing buffers */
long trace_reset_buffers_contract_0(std::string metric,
                                    std::vector<long> values) {
  long constant;
  if (metric == "instruction count")
    constant = 93;
  else if (metric == "memory instructions")
    constant = 15;
  else if (metric == "execution cycles")
    constant = 8 * DRAM_LATENCY + 7 * L1_LATENCY + 51;
  return constant;
}

/* Leaf Data Structures: */

#ifdef REHASHING_MAP
/* Rehashing map-impl */
long map_impl_init_contract(std::string metric, long success, long capacity) {
  long constant;
  if (metric == "instruction count")
    constant = 11 + 5 * capacity;
  else if (metric == "memory instructions")
    constant = 5 + 2 * capacity;
  else if (metric == "execution cycles")
    constant = 3 * DRAM_LATENCY + 2 * L1_LATENCY + 6 +
               ((2 * DRAM_LATENCY) / 16 + 3) * capacity;
  return constant;
}
long map_impl_put_contract(std::string metric, long num_traversals) {
  long constant, dynamic;
  if (metric == "instruction count") {
    dynamic = 0;
    if (num_traversals == 1) {
      constant = 43;
    } else {
      constant = 42;
      dynamic = 14 * (num_traversals - 1);
    }

  } else if (metric == "memory instructions") {
    dynamic = 0;
    if (num_traversals == 1) {
      constant = 23;
    } else {
      constant = 23;
      dynamic = 3 * (num_traversals - 1);
    }

  } else if (metric == "execution cycles") {
    constant = 6 * DRAM_LATENCY + 14 * L1_LATENCY +
               35; // Have not gone through patterns here. In progress
    dynamic = 0;
    if (num_traversals > 1) {
      num_traversals--;
      dynamic = ((2 * num_traversals) / 16 + 2) * DRAM_LATENCY +
                (((30 * num_traversals) / 16) - 2) * L1_LATENCY +
                18 * num_traversals; // The additional 2 is for misalignments
    }
  }
  return constant + dynamic;
}

long map_impl_get_contract(std::string metric, long success,
                           long num_traversals, long num_collisions) {
  long constant, dynamic;
  constant = 0;
  dynamic = 0;
  long rem_traversals;
  if (metric == "instruction count") {
    if (success) {
      if (num_traversals == 1) {
        constant = 65 + int_key_eq_contract(metric, success);
      } else {
        constant = 65 + int_key_eq_contract(metric, success);
        dynamic += num_collisions * (28 + int_key_eq_contract(metric, 0));
        rem_traversals = num_traversals - num_collisions - 1;
        dynamic += rem_traversals * 18;
      }
    } else {
      if (num_collisions == 0 &&
          num_traversals != 65536) // This should be capacity
      {
        constant = 48;
      } else if (num_traversals != 65536) {
        constant = 49;
      } else {
        constant = 52;
      }
      dynamic += num_collisions * (28 + int_key_eq_contract(metric, 0));
      rem_traversals = num_traversals - num_collisions - 1;
      dynamic += rem_traversals * 18;
    }

  } else if (metric == "memory instructions") {
    if (success) {
      if (num_traversals == 1) {
        constant = 37 + int_key_eq_contract(metric, success);
      } else {
        constant = 37 + int_key_eq_contract(metric, success);
        dynamic += num_collisions * (14 + int_key_eq_contract(metric, 0));
        rem_traversals = num_traversals - num_collisions - 1;
        dynamic += rem_traversals * 4;
      }
    } else {
      if (num_collisions == 0 &&
          num_traversals != 65536) // This should be capacity
      {
        constant = 22;
      } else if (num_traversals != 65536) {
        constant = 23;
      } else {
        constant = 23;
      }
      dynamic += num_collisions * (14 + int_key_eq_contract(metric, 0));
      rem_traversals = num_traversals - num_collisions - 1;
      dynamic += rem_traversals * 4;
    }

  } else if (metric == "execution cycles") {
    if (success) {
      constant = 12 * DRAM_LATENCY + 23 * L1_LATENCY +
                 31; // Have not gone through patterns here. In progress
    } else {
      constant = 4 * DRAM_LATENCY + 18 * L1_LATENCY +
                 29; // Can still optimise across static instances
    }
    dynamic = 0;
    if (num_traversals > 1) {
      num_traversals--;
      dynamic = ((3 * num_traversals) / 16 + 2) * DRAM_LATENCY +
                (((45 * num_traversals) / 16) - 2) * L1_LATENCY +
                18 * num_traversals; // The additional 2 is for misalignments
    }
    if (num_collisions > 0) {
      dynamic += 1 * DRAM_LATENCY + ((1 * num_collisions) / 16) * DRAM_LATENCY +
                 (24 * num_collisions) * L1_LATENCY +
                 15 * num_collisions; // Keys are 16B, so always cache aligned
    }
  }
  return constant + dynamic;
}

long map_impl_erase_contract(std::string metric, long num_traversals,
                             long num_collisions) {
  long constant, dynamic;
  constant = 0;
  dynamic = 0;
  long rem_traversals;
  if (metric == "instruction count") {
    if (num_traversals == 1) {
      constant = 69 + int_key_eq_contract(metric, 1);
    } else {
      constant = 69 + int_key_eq_contract(metric, 1);
      rem_traversals = num_traversals - num_collisions - 1;
      dynamic += num_collisions * (39 + int_key_eq_contract(metric, 0));
      dynamic += 19 * rem_traversals;
    }

  } else if (metric == "memory instructions") {
    if (num_traversals == 1) {
      constant = 43 + int_key_eq_contract(metric, 1);
    } else {
      constant = 43 + int_key_eq_contract(metric, 1);
      rem_traversals = num_traversals - num_collisions - 1;
      dynamic += num_collisions * (21 + int_key_eq_contract(metric, 0));
      dynamic += 4 * rem_traversals;
    }
  } else if (metric == "execution cycles") {
    constant = 11 * DRAM_LATENCY + 51 * L1_LATENCY + 31;
    if (num_traversals > 1) {
      num_traversals--;
      dynamic = ((3 * num_traversals) / 16 + 2) * DRAM_LATENCY +
                (((61 * num_traversals) / 16) - 2) * L1_LATENCY +
                17 * num_traversals;
    }
    if (num_collisions > 0) {
      dynamic += 1 * DRAM_LATENCY + ((1 * num_collisions) / 16) * DRAM_LATENCY +
                 (24 * num_collisions) * L1_LATENCY +
                 15 * num_collisions; // Keys are 16B, so always cache aligned
    }
  }
  return constant + dynamic;
}

#else
/*  Map_impl: */

long map_impl_init_contract(std::string metric, long success, long capacity) {
  if (success) {
    return (2 * capacity + 1) *
           DRAM_LATENCY; // Have not gone through patterns here. In progress
  } else
    return 0;
}

long map_impl_put_contract(std::string metric, long num_traversals) {
  long constant, dynamic;
  if (metric == "instruction count") {
    dynamic = 0;
    if (num_traversals == 1) {
      constant = 39;
    } else {
      constant = 38;
      dynamic = 13 * (num_traversals - 1);
    }

  } else if (metric == "memory instructions") {
    dynamic = 0;
    if (num_traversals == 1) {
      constant = 20;
    } else {
      constant = 20;
      dynamic = 2 * (num_traversals - 1);
    }

  } else if (metric == "execution cycles") {
    constant = 4 * DRAM_LATENCY + 16 * L1_LATENCY +
               32; // Have not gone through patterns here. In progress
    dynamic = 0;
    if (num_traversals > 1) {
      num_traversals--;
      dynamic = ((2 * num_traversals) / 16 + 2) * DRAM_LATENCY +
                (((30 * num_traversals) / 16) - 2) * L1_LATENCY +
                18 * num_traversals; // The additional 2 is for misalignments
    }
  }
  return constant + dynamic;
}

long map_impl_get_contract(std::string metric, long success,
                           long num_traversals, long num_collisions) {
  long constant, dynamic;
  constant = 0;
  dynamic = 0;
  long rem_traversals;
  if (metric == "instruction count") {
    if (success) {
      if (num_traversals == 1) {
        constant = 65 + int_key_eq_contract(metric, success);
      } else {
        constant = 65 + int_key_eq_contract(metric, success);
        dynamic += num_collisions * (28 + int_key_eq_contract(metric, 0));
        rem_traversals = num_traversals - num_collisions - 1;
        dynamic += rem_traversals * 18;
      }
    } else {
      if (num_collisions == 0 &&
          num_traversals != 65536) // This should be capacity
      {
        constant = 47;
      } else if (num_traversals != 65536) {
        constant = 49;
      } else {
        constant = 52;
      }
      dynamic += num_collisions * (28 + int_key_eq_contract(metric, 0));
      rem_traversals = num_traversals - num_collisions - 1;
      dynamic += rem_traversals * 18;
    }

  } else if (metric == "memory instructions") {
    if (success) {
      if (num_traversals == 1) {
        constant = 37 + int_key_eq_contract(metric, success);
      } else {
        constant = 37 + int_key_eq_contract(metric, success);
        dynamic += num_collisions * (14 + int_key_eq_contract(metric, 0));
        rem_traversals = num_traversals - num_collisions - 1;
        dynamic += rem_traversals * 4;
      }
    } else {
      if (num_collisions == 0 &&
          num_traversals != 65536) // This should be capacity
      {
        constant = 22;
      } else if (num_traversals != 65536) {
        constant = 23;
      } else {
        constant = 23;
      }
      dynamic += num_collisions * (14 + int_key_eq_contract(metric, 0));
      rem_traversals = num_traversals - num_collisions - 1;
      dynamic += rem_traversals * 4;
    }

  } else if (metric == "execution cycles") {
    if (success) {
      constant = 4 * DRAM_LATENCY + 33 * L1_LATENCY + 31 +
                 int_key_eq_contract(metric, success);
    } else {
      constant = 2 * DRAM_LATENCY + 20 * L1_LATENCY +
                 32; // Can still optimise across static instances
    }
    dynamic = 0;
    if (num_traversals > 1) {
      num_traversals--;
      dynamic = ((3 * num_traversals) / 16 + 2) * DRAM_LATENCY +
                (((45 * num_traversals) / 16) - 2) * L1_LATENCY +
                18 * num_traversals; // The additional 2 is for misalignments
    }
    if (num_collisions > 0) {
      dynamic += 1 * DRAM_LATENCY + ((1 * num_collisions) / 16) * DRAM_LATENCY +
                 (24 * num_collisions) * L1_LATENCY +
                 15 * num_collisions; // Keys are 16B, so always cache aligned
    }
  }
  return constant + dynamic;
}

long map_impl_erase_contract(std::string metric, long num_traversals,
                             long num_collisions) {
  long constant, dynamic;
  constant = 0;
  dynamic = 0;
  long rem_traversals;
  if (metric == "instruction count") {
    if (num_traversals == 1) {
      constant = 69 + int_key_eq_contract(metric, 1);
    } else {
      constant = 69 + int_key_eq_contract(metric, 1);
      rem_traversals = num_traversals - num_collisions - 1;
      dynamic += num_collisions * (39 + int_key_eq_contract(metric, 0));
      dynamic += 19 * rem_traversals;
    }

  } else if (metric == "memory instructions") {
    if (num_traversals == 1) {
      constant = 43 + int_key_eq_contract(metric, 1);
    } else {
      constant = 43 + int_key_eq_contract(metric, 1);
      rem_traversals = num_traversals - num_collisions - 1;
      dynamic += num_collisions * (21 + int_key_eq_contract(metric, 0));
      dynamic += 4 * rem_traversals;
    }
  } else if (metric == "execution cycles") {
    constant = 4 * DRAM_LATENCY + 39 * L1_LATENCY + 31 +
               int_key_eq_contract(metric, 1);
    if (num_traversals > 1) {
      num_traversals--;
      dynamic = ((3 * num_traversals) / 16 + 2) * DRAM_LATENCY +
                (((61 * num_traversals) / 16) - 2) * L1_LATENCY +
                17 * num_traversals;
    }
    if (num_collisions > 0) {
      dynamic += 1 * DRAM_LATENCY + ((1 * num_collisions) / 16) * DRAM_LATENCY +
                 (24 * num_collisions) * L1_LATENCY +
                 15 * num_collisions; // Keys are 16B, so always cache aligned
    }
  }
  return constant + dynamic;
}
#endif
#ifdef ALT_CHAIN

/* Alternate chain */
long dchain_expire_one_index_contract(std::string metric, long success) {
  long constant;
  if (metric == "instruction count") {
    if (success) {
      constant = 25;
    } else {
      constant = 15;
    }
  } else if (metric == "memory instructions") {
    if (success) {
      constant = 10;
    } else {
      constant = 5;
    }
  } else if (metric == "execution cycles") {
    if (success) {
      constant = (9) * DRAM_LATENCY + 16 * L1_LATENCY + 28; // Have not gone
                                                            // through patterns
                                                            // here. In progress
                                                            // - Not of much use
    } else // We can reduce this to 9 if we condition on empty list
    {
      constant = (3) * DRAM_LATENCY + 2 * L1_LATENCY +
                 18; // Have not gone through patterns here. In progress
    }
  }
  return constant;
}

long dchain_allocate_contract_0(std::string metric, std::vector<long> values) {
  //     long success = values[0];
  //     long capacity =  values[1];
  //     assert(success);
  //     long constant = (23 + 2*(capacity -1))*DRAM_LATENCY; // Have not gone
  // through patterns here. In progress
  //     return constant;
  return 0;
}
long dchain_allocate_contract_1(std::string metric, std::vector<long> values) {
  //     long success = values[0];
  //     assert(success==0);
  //     long constant = 0;
  //     return constant;
  return 0;
}
long dchain_allocate_new_index_contract_0(std::string metric,
                                          std::vector<long> values) {
  long success = values[0];
  assert(success == 0);
  long constant;
  if (metric == "instruction count") {
    constant = 36;
  } else if (metric == "memory instructions") {
    constant = 14;
  } else if (metric == "execution cycles") {
    constant = (5) * DRAM_LATENCY + 9 * L1_LATENCY + 38;
  }
  return constant;
}
long dchain_allocate_new_index_contract_1(std::string metric,
                                          std::vector<long> values) {
  long success = values[0];
  assert(success);
  long constant;
  if (metric == "instruction count") {
    constant = 393224;
  } else if (metric == "memory instructions") {
    constant = 65540;
  } else if (metric == "execution cycles") {
    constant = (3) * DRAM_LATENCY + 7 * L1_LATENCY +
               11; // Have not gone through patterns here. In progress
  }
  return constant;
}
long dchain_rejuvenate_index_contract_0(std::string metric,
                                        std::vector<long> values) {
  long constant;
  if (metric == "instruction count") {
    constant = 29;
  } else if (metric == "memory instructions") {
    constant = 14;
  } else if (metric == "execution cycles") {
    constant = (9) * DRAM_LATENCY + 11 * L1_LATENCY +
               18; // Have not gone through patterns here. In progress
  }
  return constant;
}
#else
/* Double chain */

long dchain_is_index_allocated_contract(std::string metric) {
  long constant;
  if (metric == "instruction count") {
    constant = 13;
  } else if (metric == "memory instructions") {
    constant = 4;
  } else if (metric == "execution cycles") {
    constant = (1) * DRAM_LATENCY + 3 * L1_LATENCY +
               9; // Have not gone through patterns here. In progress
  }
  return constant;
}

long dchain_expire_one_index_contract(std::string metric, long success) {
  long constant;
  if (metric == "instruction count") {
    if (success) {
      constant = 50;
    } else {
      constant = 30;
    }
  } else if (metric == "memory instructions") {
    if (success) {
      constant = 25;
    } else {
      constant = 15;
    }
  } else if (metric == "execution cycles") {
    if (success) {
      constant = (2) * DRAM_LATENCY + 23 * L1_LATENCY + 28; // Have not gone
                                                            // through patterns
                                                            // here. In progress
                                                            // - Not of much use
    } else // We can reduce this to 9 if we condition on empty list
    {
      constant = (2) * DRAM_LATENCY + 13 * L1_LATENCY +
                 16; // Have not gone through patterns here. In progress
    }
  }
  return constant;
}

long dchain_allocate_contract_0(std::string metric, std::vector<long> values) {
  //     long success = values[0];
  //     long capacity =  values[1];
  //     assert(success);
  //     long constant = (23 + 2*(capacity -1))*DRAM_LATENCY; // Have not gone
  // through patterns here. In progress
  //     return constant;
  return 0;
}
long dchain_allocate_contract_1(std::string metric, std::vector<long> values) {
  //     long success = values[0];
  //     assert(success==0);
  //     long constant = 0;
  //     return constant;
  return 0;
}
long dchain_allocate_new_index_contract_0(std::string metric,
                                          std::vector<long> values) {
  long success = values[0];
  assert(success == 0); /* success is the inverse here*/
  long constant;
  if (metric == "instruction count") {
    constant = 35;
  } else if (metric == "memory instructions") {
    constant = 23;
  } else if (metric == "execution cycles") {
    constant = (3) * DRAM_LATENCY + 20 * L1_LATENCY + 22;
  }
  return constant;
}
long dchain_allocate_new_index_contract_1(std::string metric,
                                          std::vector<long> values) {
  long success = values[0];
  assert(success); /* success is the inverse here*/
  long constant;
  if (metric == "instruction count") {
    constant = 19;
  } else if (metric == "memory instructions") {
    constant = 10;
  } else if (metric == "execution cycles") {
    constant = (3) * DRAM_LATENCY + 7 * L1_LATENCY +
               11; // Have not gone through patterns here. In progress
  }
  return constant;
}
long dchain_rejuvenate_index_contract_0(std::string metric,
                                        std::vector<long> values) {
  long constant;
  if (metric == "instruction count") {
    constant = 34;
  } else if (metric == "memory instructions") {
    constant = 20;
  } else if (metric == "execution cycles") {
    constant = (3) * DRAM_LATENCY + 17 * L1_LATENCY + 18;
  }
  return constant;
}
#endif

/*  Vector  */
long vector_allocate_contract_0(std::string metric, std::vector<long> values) {
  return 0;
}

long vector_borrow_contract_0(std::string metric, std::vector<long> values) {
  long constant;
  if (metric == "instruction count") {
    constant = 5;
  } else if (metric == "memory instructions") {
    constant = 5;
  } else if (metric == "execution cycles") {
    constant = 1 * DRAM_LATENCY + 4 * L1_LATENCY + 2;
  }
  return constant;
}

long vector_return_contract_0(std::string metric, std::vector<long> values) {
  long constant;
  if (metric == "instruction count") {
    constant = 1;
  } else if (metric == "memory instructions") {
    constant = 2;
  } else if (metric == "execution cycles") {
    constant = 1 + 2 * L1_LATENCY;
  }
  return constant;
}

/*  IP Options Router */

long handle_packet_timestamp_contract_0(std::string metric,
                                        std::vector<long> values) {
  long constant;
  if (metric == "instruction count") {
    constant = 303;
  } else if (metric == "memory instructions") {
    constant = 66;
  } else if (metric == "execution cycles") {
    constant = 20 * DRAM_LATENCY + 46 * L1_LATENCY + 300;
  }
  return constant;
}
/* DPDK LPM */
long lpm_init_contract_0(std::string metric, std::vector<long> values) {
  return 0;
}
long lpm_lookup_contract_0(std::string metric, std::vector<long> values) {
  long lpm_stages = values[0];
  long constant;
  if (metric == "instruction count") {
    constant = (lpm_stages == 2) ? 19 : 13;
  } else if (metric == "memory instructions") {
    constant = (lpm_stages == 2) ? 4 : 2;
  } else if (metric == "execution cycles") {
    constant = (lpm_stages == 2) ? 3 * DRAM_LATENCY + 1 * L1_LATENCY + 15
                                 : 1 * DRAM_LATENCY + 1 * L1_LATENCY + 11;
  }
  return constant;
}
/* Composite Data Structures: */

#ifdef REHASHING_MAP
/* Rehashing Map */
long map_allocate_contract_0(std::string metric, std::vector<long> values) {
  //    long success = values[0];
  //    long capacity = values[1];
  //    assert(success);
  //    long constant = 40*DRAM_LATENCY; // Have not gone through patterns here.
  // In progress
  //    long dependency = map_impl_init_contract(success,capacity);
  //    return constant+dependency;
  return 0;
}

long map_allocate_contract_1(std::string metric, std::vector<long> values) {
  //    long success = values[0];
  //    long capacity = values[1];
  //    assert(success==0);
  //    long constant = 0;
  //    long dependency = map_impl_init_contract(success,capacity);
  //    return constant+dependency;
  return 0;
}

long map_get_contract_0(std::string metric, std::vector<long> values) {
  long num_traversals = values[0];
  long num_collisions = values[1];
  long success = values[2];
  long constant, dependency;
  assert(success);
  if (metric == "instruction count") {
    constant = 30;
  } else if (metric == "memory instructions") {
    constant = 24;
  } else if (metric == "execution cycles") {
    constant = 12 * DRAM_LATENCY + 12 * L1_LATENCY +
               8; // Have not gone through patterns here. In progress
  }
  dependency =
      map_impl_get_contract(metric, success, num_traversals, num_collisions);
  dependency += int_key_hash_contract(metric);
  return constant + dependency;
}
long map_get_contract_1(std::string metric, std::vector<long> values) {
  long num_traversals = values[0];
  long num_collisions = values[1];
  long success = values[2];
  long constant, dependency;
  assert(success == 0);
  if (metric == "instruction count") {
    constant = 30;
  } else if (metric == "memory instructions") {
    constant = 24;
  } else if (metric == "execution cycles") {
    constant = 12 * DRAM_LATENCY + 12 * L1_LATENCY +
               8; // Have not gone through patterns here. In progress
  }
  dependency =
      map_impl_get_contract(metric, success, num_traversals, num_collisions);
  dependency += int_key_hash_contract(metric);
  return constant + dependency;
}

long map_put_contract_0(std::string metric, std::vector<long> values) {
  long num_traversals = values[0];
  long map_occupancy = values[1];
  long constant, dynamic, constant_dependency, dynamic_dependency;
  if (num_traversals > REHASHING_THRESHOLD) {

    if (metric == "instruction count") {
      constant = 90;
      dynamic = 10 * 65536;
      dynamic += 34 * map_occupancy;
    } else if (metric == "memory instructions") {
      constant = 58;
      dynamic = 4 * 65536;
      dynamic += 24 * map_occupancy;
    } else if (metric == "execution cycles") {
      constant = 7 * DRAM_LATENCY + 12 * L1_LATENCY +
                 9; // Have not gone through patterns here. In progress
      dynamic = (2 * DRAM_LATENCY + 2 * L1_LATENCY + 6) * 65536;
      dynamic += (12 * DRAM_LATENCY + 12 * L1_LATENCY + 10) * map_occupancy;
    }
    constant_dependency = map_impl_put_contract(metric, num_traversals);
    constant_dependency += int_key_hash_contract(metric);
    constant_dependency += map_impl_init_contract(metric, 1, 65536);
    dynamic_dependency =
        map_impl_put_contract(metric, num_traversals) * map_occupancy;
    dynamic_dependency += int_key_hash_contract(metric) * map_occupancy;
  } else {
    if (metric == "instruction count") {
      constant = 53;
    } else if (metric == "memory instructions") {
      constant = 38;
    } else if (metric == "execution cycles") {
      constant = 19 * DRAM_LATENCY + 19 * L1_LATENCY +
                 17; // Have not gone through patterns here. In progress
    }
    dynamic = 0;
    constant_dependency = map_impl_put_contract(metric, num_traversals);
    constant_dependency += int_key_hash_contract(metric);
    dynamic_dependency = 0;
  }

  return constant + dynamic + constant_dependency + dynamic_dependency;
}

long map_erase_contract_0(std::string metric, std::vector<long> values) {
  long num_traversals = values[0];
  long num_collisions = values[1];
  long constant, dependency;
  if (metric == "instruction count") {
    constant = 33;
  } else if (metric == "memory instructions") {
    constant = 26;
  } else if (metric == "execution cycles") {
    constant = 14 * DRAM_LATENCY + 12 * L1_LATENCY + 9; // Needs to be updated
  }
  dependency = map_impl_erase_contract(metric, num_traversals, num_collisions);
  dependency += int_key_hash_contract(metric);
  return constant + dependency;
}

#else
/*  Map: */

long map_allocate_contract_0(std::string metric, std::vector<long> values) {
  //    long success = values[0];
  //    long capacity = values[1];
  //    assert(success);
  //    long constant = 40*DRAM_LATENCY; // Have not gone through patterns here.
  // In progress
  //    long dependency = map_impl_init_contract(success,capacity);
  //    return constant+dependency;
  return 0;
}

long map_allocate_contract_1(std::string metric, std::vector<long> values) {
  //    long success = values[0];
  //    long capacity = values[1];
  //    assert(success==0);
  //    long constant = 0;
  //    long dependency = map_impl_init_contract(success,capacity);
  //    return constant+dependency;
  return 0;
}

long map_get_contract_0(std::string metric, std::vector<long> values) {
  long num_traversals = values[0];
  long num_collisions = values[1];
  long success = values[2];
  long constant, dependency;
  assert(success);
  if (metric == "instruction count") {
    constant = 25;
  } else if (metric == "memory instructions") {
    constant = 19;
  } else if (metric == "execution cycles") {
    constant = 0 * DRAM_LATENCY + 19 * L1_LATENCY + 8;
  }
  dependency = map_impl_get_contract(
      metric, success, num_traversals,
      num_collisions); /* This currently uses the NAT equality fn */
  dependency +=
      ether_addr_hash_contract(metric); /* Bridge, need separate for LB */
  return constant + dependency;
}
long map_get_contract_1(std::string metric, std::vector<long> values) {
  long num_traversals = values[0];
  long num_collisions = values[1];
  long success = values[2];
  long constant, dependency;
  assert(success == 0);
  if (metric == "instruction count") {
    constant = 25;
  } else if (metric == "memory instructions") {
    constant = 19;
  } else if (metric == "execution cycles") {
    constant = 0 * DRAM_LATENCY + 19 * L1_LATENCY + 8;
  }
  dependency = map_impl_get_contract(
      metric, success, num_traversals,
      num_collisions); /* This currently uses the NAT equality fn */
  dependency +=
      ether_addr_hash_contract(metric); /* Bridge need separate for LB */
  return constant + dependency;
}

long map_put_contract_0(std::string metric, std::vector<long> values) {
  long num_traversals = values[0];
  long constant, dependency;
  if (metric == "instruction count") {
    constant = 26;
  } else if (metric == "memory instructions") {
    constant = 19;
  } else if (metric == "execution cycles") {
    constant = 0 * DRAM_LATENCY + 19 * L1_LATENCY + 9;
  }
  dependency = map_impl_put_contract(metric, num_traversals);
  dependency +=
      ether_addr_hash_contract(metric); /* Bridge need separate for LB */
  return constant + dependency;
}

long map_erase_contract_0(std::string metric, std::vector<long> values) {
  long num_traversals = values[0];
  long num_collisions = values[1];
  long constant, dependency;
  if (metric == "instruction count") {
    constant = 26;
  } else if (metric == "memory instructions") {
    constant = 19;
  } else if (metric == "execution cycles") {
    constant = 0 * DRAM_LATENCY + 19 * L1_LATENCY + 9;
  }
  dependency = map_impl_erase_contract(metric, num_traversals, num_collisions);
  dependency +=
      ether_addr_hash_contract(metric); /* Bridge need separate for LB */
  return constant + dependency;
}
#endif

/*  Double Map: */
long dmap_allocate_contract_0(std::string metric, std::vector<long> values) {
  //    long success = values[0];
  //    long capacity = values[1];
  //    assert(success);
  //     long constant = 91*DRAM_LATENCY;  // Have not gone through patterns
  // here. In progress
  //     long dependency = 2*map_impl_init_contract(success,capacity);
  //     return constant+dependency;
  return 0;
}

long dmap_allocate_contract_1(std::string metric, std::vector<long> values) {
  //    long success = values[0];
  //    long capacity = values[1];
  //    assert(success==0);
  //     long constant = 0;
  //     long dependency = 2*map_impl_init_contract(success,capacity);
  //     return constant+dependency;
  return 0;
}

long dmap_get_a_contract_0(std::string metric, std::vector<long> values) {
  long num_traversals_a = values[0];
  long num_collisions_a = values[1];
  long success = values[2];
  long constant, dependency;
  assert(success);
  if (metric == "instruction count") {
    constant = 25;
  } else if (metric == "memory instructions") {
    constant = 19;
  } else if (metric == "execution cycles") {
    constant = 0 * DRAM_LATENCY + 19 * L1_LATENCY + 8;
  }
  dependency = map_impl_get_contract(metric, success, num_traversals_a,
                                     num_collisions_a);
  dependency += int_key_hash_contract(metric);
  return constant + dependency;
}

long dmap_get_a_contract_1(std::string metric, std::vector<long> values) {
  long num_traversals_a = values[0];
  long num_collisions_a = values[1];
  long success = values[2];
  long constant, dependency;
  assert(success == 0);
  if (metric == "instruction count") {
    constant = 25;
  } else if (metric == "memory instructions") {
    constant = 19;
  } else if (metric == "execution cycles") {
    constant = 0 * DRAM_LATENCY + 19 * L1_LATENCY + 8;
  }
  dependency = map_impl_get_contract(metric, success, num_traversals_a,
                                     num_collisions_a);
  dependency += int_key_hash_contract(metric);
  return constant + dependency;
}

long dmap_get_b_contract_0(std::string metric, std::vector<long> values) {
  long num_traversals_b = values[0];
  long num_collisions_b = values[1];
  long success = values[2];
  long constant, dependency;
  assert(success);
  if (metric == "instruction count") {
    constant = 25;
  } else if (metric == "memory instructions") {
    constant = 19;
  } else if (metric == "execution cycles") {
    constant = 0 * DRAM_LATENCY + 19 * L1_LATENCY + 8;
  }
  dependency = map_impl_get_contract(metric, success, num_traversals_b,
                                     num_collisions_b);
  dependency += ext_key_hash_contract(metric);
  return constant + dependency;
}

long dmap_get_b_contract_1(std::string metric, std::vector<long> values) {
  long num_traversals_b = values[0];
  long num_collisions_b = values[1];
  long success = values[2];
  long constant, dependency;
  assert(success == 0);
  if (metric == "instruction count") {
    constant = 25;
  } else if (metric == "memory instructions") {
    constant = 19;
  } else if (metric == "execution cycles") {
    constant = 0 * DRAM_LATENCY + 19 * L1_LATENCY + 8;
  }
  dependency = map_impl_get_contract(metric, success, num_traversals_b,
                                     num_collisions_b);
  dependency += int_key_hash_contract(metric);
  return constant + dependency;
}
long dmap_get_value_contract_0(std::string metric, std::vector<long> values) {
  long constant, dependency;
  if (metric == "instruction count") {
    constant = 6;
  } else if (metric == "memory instructions") {
    constant = 3;
  } else if (metric == "execution cycles") {
    constant = 1 * DRAM_LATENCY + 2 * L1_LATENCY + 3;
  }
  dependency = flow_cpy_contract(metric);
  return constant + dependency;
}

long dmap_put_contract_0(std::string metric, std::vector<long> values) {
  long num_traversals_a = values[0];
  long num_traversals_b = values[1];
  long constant, dependency;
  if (metric == "instruction count") {
    constant = 65;
  } else if (metric == "memory instructions") {
    constant = 45;
  } else if (metric == "execution cycles") {
    constant = 0 * DRAM_LATENCY + 45 * L1_LATENCY + 27;
  }
  dependency = map_impl_put_contract(metric, num_traversals_a);
  dependency += map_impl_put_contract(metric, num_traversals_b);
  dependency += flow_cpy_contract(metric);
  dependency += flow_extract_keys_contract(metric);
  dependency += flow_pack_keys_contract(metric);
  dependency += int_key_hash_contract(metric);
  dependency += ext_key_hash_contract(metric);
  return constant + dependency;
}

long dmap_erase_contract_0(std::string metric, std::vector<long> values) {
  long num_traversals_a = values[0];
  long num_traversals_b = values[1];
  long num_collisions_a = values[2];
  long num_collisions_b = values[3];
  long constant, dependency;
  if (metric == "instruction count") {
    constant = 72;
  } else if (metric == "memory instructions") {
    constant = 48;
  } else if (metric == "execution cycles") {
    constant = 0 * DRAM_LATENCY + 48 * L1_LATENCY + 31;
  }
  dependency =
      map_impl_erase_contract(metric, num_traversals_a, num_collisions_a);
  dependency +=
      map_impl_erase_contract(metric, num_traversals_b, num_collisions_b);
  dependency += flow_extract_keys_contract(metric);
  dependency += flow_pack_keys_contract(metric);
  dependency += flow_pack_keys_contract(metric);
  dependency += int_key_hash_contract(metric);
  dependency += ext_key_hash_contract(metric);
  dependency += flow_destroy_contract(metric);
  return constant + dependency;
}

/*  Expirator  */

long expire_items_contract_0(std::string metric, std::vector<long> values) {
  long n_expired = values[4];
  long constant, constant_dependency, dynamic, dynamic_dependency;
  dynamic = 0;
  dynamic_dependency = 0;
  if (metric == "instruction count") {
    constant = 30;
    dynamic = 10 * n_expired;
  } else if (metric == "memory instructions") {
    constant = 13;
    dynamic = 1 * n_expired;
  } else if (metric == "execution cycles") {
    constant = 0 * DRAM_LATENCY + 13 * L1_LATENCY + 21;
    dynamic = (1 * L1_LATENCY + 11) * n_expired;
  }
  constant_dependency =
      dchain_expire_one_index_contract(metric, 0); // Eventually fail to expire
  long num_collisions_a = values[2];
  long num_collisions_b = values[3];
  long max_collisions = std::max(num_collisions_a, num_collisions_b);

  if (n_expired > 0) {
    // Bounding the unique flows
    values[2] = 0;
    values[3] = 0;
    long unique_flow_hashes = (n_expired - max_collisions - 1) > 0
                                  ? (n_expired - max_collisions - 1)
                                  : 0;
    dynamic_dependency += dchain_expire_one_index_contract(metric, 1);
    dynamic_dependency += dmap_erase_contract_0(metric, values);
    dynamic_dependency *= unique_flow_hashes;

    values[2] = num_collisions_a;
    values[3] = num_collisions_b;
    int penalised_flows =
        n_expired >= (max_collisions + 1) ? max_collisions + 1 : n_expired;
    for (int i = 0; i < penalised_flows; i++) {
      dynamic_dependency += dchain_expire_one_index_contract(metric, 1);
      dynamic_dependency += dmap_erase_contract_0(metric, values);
      values[0]--;
      values[1]--;
      values[2]--;
      values[3]--;

      for (std::vector<long>::iterator iter = values.begin();
           iter != values.end(); ++iter) {
        if (*iter < 0) {
          *iter = 0;
        }
      }
    }
  }
  return (constant + constant_dependency + dynamic + dynamic_dependency);
}

long expire_items_single_map_contract_0(std::string metric,
                                        std::vector<long> values) {
  long n_expired = values[2];
  long constant, constant_dependency, dynamic, dynamic_dependency;
  dynamic_dependency = 0;
  if (metric == "instruction count") {
    constant = 33;
    dynamic = 19 * n_expired;
  } else if (metric == "memory instructions") {
    constant = 15;
    dynamic = 4 * n_expired;
  } else if (metric == "execution cycles") {

    constant = 0 * DRAM_LATENCY + 15 * L1_LATENCY +
               22; // Have not gone through patterns here. In progress
    dynamic = (4 * L1_LATENCY + 21) * n_expired;
  }
  constant_dependency =
      dchain_expire_one_index_contract(metric, 0); // Eventually fail to expire
  long num_collisions = values[1];

  if (n_expired > 0) {
    // Bounding the unique flows
    values[1] = 0;
    long unique_flow_hashes = (n_expired - num_collisions - 1) > 0
                                  ? (n_expired - num_collisions - 1)
                                  : 0;
    dynamic_dependency += dchain_expire_one_index_contract(metric, 1);
    dynamic_dependency += map_erase_contract_0(metric, values);
    dynamic_dependency += vector_borrow_contract_0(metric, values);
    dynamic_dependency += vector_return_contract_0(metric, values);

    dynamic_dependency *= unique_flow_hashes;

    values[1] = num_collisions;
    int penalised_flows =
        n_expired >= (num_collisions + 1) ? num_collisions + 1 : n_expired;
    for (int i = 0; i < penalised_flows; i++) {
      dynamic_dependency += dchain_expire_one_index_contract(metric, 1);
      dynamic_dependency += map_erase_contract_0(metric, values);
      dynamic_dependency += vector_borrow_contract_0(metric, values);
      dynamic_dependency += vector_return_contract_0(metric, values);
      values[0]--;
      values[1]--;

      for (std::vector<long>::iterator iter = values.begin();
           iter != values.end(); ++iter) {
        if (*iter < 0) {
          *iter = 0;
        }
      }
    }
  }
  return (constant + constant_dependency + dynamic + dynamic_dependency);
}

long lb_find_preferred_available_backend_contract_0(std::string metric,
                                                    std::vector<long> values) {
  long available_backends = values[0];
  long constant = 0;
  if (metric == "instruction count") {
    constant = 55;
  } else if (metric == "memory instructions") {
    constant = 25;
  } else if (metric == "execution cycles") {
    constant = 0 * DRAM_LATENCY + 25 * L1_LATENCY + 31;
  }
  long constant_dependency = vector_borrow_contract_0(metric, values);
  constant_dependency += vector_return_contract_0(metric, values);
  constant_dependency += dchain_is_index_allocated_contract(metric);
  return constant +
         constant_dependency; /*Hack, assuming backends table is full*/
}

long flood_contract_0(std::string metric, std::vector<long> values) {
  long constant = 0;
  if (metric == "instruction count") {
    constant = 309;
  } else if (metric == "memory instructions") {
    constant = 177;
  } else if (metric == "execution cycles") {
    constant = 4 * DRAM_LATENCY + 173 * L1_LATENCY + 147;
  }
  return constant;
}

long model_functions_contract_0(std::string metric, std::vector<long> values) {
  return 0;
}

/* Contracts for concrete state */
std::map<std::string, std::set<int> >
add_cstate_dependency(std::map<std::string, std::set<int> > caller,
                      std::map<std::string, int> caller_state,
                      std::map<std::string, std::set<int> > callee) {

  std::map<std::string, std::set<int> > temp;
  for (auto it : caller_state) {
    if (callee.find(it.first) != callee.end()) {
      for (auto it1 : callee[it.first]) {
        temp[it.first].insert(it1 + it.second);
      }
    }
  }

  for (auto it : caller) {
    std::set<int> tempset;
    std::set_union(std::begin(it.second), std::end(it.second),
                   std::begin(temp[it.first]), std::end(temp[it.first]),
                   std::inserter(tempset, std::begin(tempset)));
    caller[it.first] = tempset;
  }

  return caller;
}

std::map<std::string, std::set<int> > ether_addr_hash_cstate_contract() {

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}

std::map<std::string, std::set<int> > ether_addr_eq_cstate_contract() {

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}

std::map<std::string, std::set<int> > int_key_hash_cstate_contract() {

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}

std::map<std::string, std::set<int> > ext_key_hash_cstate_contract() {

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}

std::map<std::string, std::set<int> > int_key_eq_cstate_contract() {

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}

std::map<std::string, std::set<int> > ext_key_eq_cstate_contract() {

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}

std::map<std::string, std::set<int> > flow_cpy_cstate_contract() {

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}

std::map<std::string, std::set<int> > flow_destroy_cstate_contract() {

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}

std::map<std::string, std::set<int> > flow_extract_keys_cstate_contract() {

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}

std::map<std::string, std::set<int> > flow_pack_keys_cstate_contract() {

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}

std::map<std::string, std::set<int> >
trace_reset_buffers_cstate_contract_0(std::vector<long> values) {

  /* TODO */
  std::map<std::string, std::set<int> > cstate;
  return cstate;
}

/* Leaf Data Structures: */

#ifdef REHASHING_MAP
/* Rehashing map-impl */
std::map<std::string, std::set<int> >
map_impl_init_cstate_contract(long success, long capacity) {

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}
std::map<std::string, std::set<int> >
map_impl_put_cstate_contract(long num_traversals) {

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}

std::map<std::string, std::set<int> >
map_impl_get_cstate_contract(long success, long num_traversals,
                             long num_collisions) {

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}

std::map<std::string, std::set<int> >
map_impl_erase_cstate_contract(long num_traversals, long num_collisions) {

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}

#else
/*  Map_impl: */

std::map<std::string, std::set<int> >
map_impl_init_cstate_contract(long success, long capacity) {

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}

std::map<std::string, std::set<int> >
map_impl_put_cstate_contract(long num_traversals) {

  std::map<std::string, std::set<int> > cstate;
  cstate["rsp"] = { -8, -16, -24, -32, -40, 32, 16, 24 };
  return cstate;
}

std::map<std::string, std::set<int> >
map_impl_get_cstate_contract(long success, long num_traversals,
                             long num_collisions) {

  std::map<std::string, std::set<int> > cstate;
  if (success) {
    cstate["rsp"] = { -8,  -16, -24, -32, -40, 32,   16,  -80, -64, -56,
                      -80, -84, -88, -96, -72, -104, -64, -48, 24 };
    std::map<std::string, int> dependency_calls;
    dependency_calls["rsp"] = -104;
    cstate = add_cstate_dependency(
        cstate, dependency_calls,
        int_key_eq_cstate_contract()); /*This is for the NAT*/
  } else {
    cstate["rsp"] = { -8, -16, -24, -32, -40, 32, 16, -80, -64, -56 };
  }
  return cstate;
}

std::map<std::string, std::set<int> >
map_impl_erase_cstate_contract(long num_traversals, long num_collisions) {

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}
#endif
#ifdef ALT_CHAIN

/* Alternate chain */
std::map<std::string, std::set<int> >
dchain_expire_one_index_cstate_contract(long success) {

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}

std::map<std::string, std::set<int> >
dchain_allocate_cstate_contract_0(std::vector<long> values) {

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}
std::map<std::string, std::set<int> >
dchain_allocate_cstate_contract_1(std::vector<long> values) {

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}

std::map<std::string, std::set<int> >
dchain_allocate_new_index_cstate_contract_0(std::vector<long> values) {

  long success = values[0];
  assert(success == 0);

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}
std::map<std::string, std::set<int> >
dchain_allocate_new_index_cstate_contract_1(std::vector<long> values) {

  long success = values[0];
  assert(success);

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}
std::map<std::string, std::set<int> >
dchain_rejuvenate_index_cstate_contract_0(std::vector<long> values) {

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}
#else
/* Double chain */
std::map<std::string, std::set<int> >
dchain_is_index_allocated_cstate_contract() {

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}
std::map<std::string, std::set<int> >
dchain_expire_one_index_cstate_contract(long success) {

  std::map<std::string, std::set<int> > cstate;
  if (!success) {
    cstate["rsp"] = { -8, -16, -24, -32, -32 };
  }
  return cstate;
}
std::map<std::string, std::set<int> >
dchain_allocate_cstate_contract_0(std::vector<long> values) {

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}
std::map<std::string, std::set<int> >
dchain_allocate_cstate_contract_1(std::vector<long> values) {

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}
std::map<std::string, std::set<int> >
dchain_allocate_new_index_cstate_contract_0(std::vector<long> values) {

  long success = values[0];
  assert(success == 0); /* success is the inverse here*/

  std::map<std::string, std::set<int> > cstate;
  cstate["rsp"] = { -8, -16, -24, -32, -40 };
  return cstate;
}
std::map<std::string, std::set<int> >
dchain_allocate_new_index_cstate_contract_1(std::vector<long> values) {

  long success = values[0];
  assert(success); /* success is the inverse here*/

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}
std::map<std::string, std::set<int> >
dchain_rejuvenate_index_cstate_contract_0(std::vector<long> values) {

  std::map<std::string, std::set<int> > cstate;
  cstate["rsp"] = { -8, -16, -24, -32 };
  return cstate;
}
#endif

/*  Vector  */
std::map<std::string, std::set<int> >
vector_allocate_cstate_contract_0(std::vector<long> values) {

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}

std::map<std::string, std::set<int> >
vector_borrow_cstate_contract_0(std::vector<long> values) {

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}

std::map<std::string, std::set<int> >
vector_return_cstate_contract_0(std::vector<long> values) {

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}

/*  IP Options Router */

std::map<std::string, std::set<int> >
handle_packet_timestamp_cstate_contract_0(std::vector<long> values) {

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}
/* DPDK LPM */
std::map<std::string, std::set<int> >
lpm_init_cstate_contract_0(std::vector<long> values) {

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}
std::map<std::string, std::set<int> >
lpm_lookup_cstate_contract_0(std::vector<long> values) {

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}
/* Composite Data Structures: */

#ifdef REHASHING_MAP
/* Rehashing Map */
std::map<std::string, std::set<int> >
map_allocate_cstate_contract_0(std::vector<long> values) {

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}

std::map<std::string, std::set<int> >
map_allocate_cstate_contract_1(std::vector<long> values) {

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}

std::map<std::string, std::set<int> >
map_get_cstate_contract_0(std::vector<long> values) {

  long num_traversals = values[0];
  long num_collisions = values[1];
  long success = values[2];
  assert(success);

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}
std::map<std::string, std::set<int> >
map_get_cstate_contract_1(std::vector<long> values) {

  long num_traversals = values[0];
  long num_collisions = values[1];
  long success = values[2];
  assert(success == 0);

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}

std::map<std::string, std::set<int> >
map_put_cstate_contract_0(std::vector<long> values) {

  long num_traversals = values[0];
  long map_occupancy = values[1];

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}

std::map<std::string, std::set<int> >
map_erase_cstate_contract_0(std::vector<long> values) {

  long num_traversals = values[0];
  long num_collisions = values[1];

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}

#else
/*  Map: */

std::map<std::string, std::set<int> >
map_allocate_cstate_contract_0(std::vector<long> values) {

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}

std::map<std::string, std::set<int> >
map_allocate_cstate_contract_1(std::vector<long> values) {

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}

std::map<std::string, std::set<int> >
map_get_cstate_contract_0(std::vector<long> values) {

  long num_traversals = values[0];
  long num_collisions = values[1];
  long success = values[2];
  assert(success);
  std::map<std::string, std::set<int> > cstate;

  cstate["rsp"] = { -8, -16, -24, -32, -40, -48, -56, -64 };
  cstate["rbx"] = { 40, 24, 16, 8, 32, 0 };

  std::map<std::string, int> dependency_calls;
  dependency_calls["rsp"] = -64;
  dependency_calls["rbx"] = 0;
  cstate = add_cstate_dependency(
      cstate, dependency_calls,
      map_impl_get_cstate_contract(success, num_traversals, num_collisions));
  dependency_calls["rsp"] = -32;
  cstate =
      add_cstate_dependency(cstate, dependency_calls,
                            ether_addr_hash_cstate_contract()); /*For bridge*/

  return cstate;
}
std::map<std::string, std::set<int> >
map_get_cstate_contract_1(std::vector<long> values) {

  long num_traversals = values[0];
  long num_collisions = values[1];
  long success = values[2];
  assert(success == 0);
  std::map<std::string, std::set<int> > cstate;

  cstate["rsp"] = { -8, -16, -24, -32, -40, -48, -56, -64 };
  cstate["rbx"] = { 40, 24, 16, 8, 32, 0 };
  std::map<std::string, int> dependency_calls;
  dependency_calls["rsp"] = -64;
  dependency_calls["rbx"] = 0;
  cstate = add_cstate_dependency(
      cstate, dependency_calls,
      map_impl_get_cstate_contract(success, num_traversals, num_collisions));
  dependency_calls["rsp"] = -32;
  cstate =
      add_cstate_dependency(cstate, dependency_calls,
                            ether_addr_hash_cstate_contract()); /*For bridge*/

  return cstate;
}

std::map<std::string, std::set<int> >
map_put_cstate_contract_0(std::vector<long> values) {

  long num_traversals = values[0];
  std::map<std::string, std::set<int> > cstate;

  cstate["rsp"] = { -8, -16, -24, -32, -40, -48, -56, -64 };
  cstate["rbx"] = { 40, 24, 16, 8, 32, 0, 44 };
  std::map<std::string, int> dependency_calls;
  dependency_calls["rsp"] = -64;
  dependency_calls["rbx"] = 0;
  cstate = add_cstate_dependency(cstate, dependency_calls,
                                 map_impl_put_cstate_contract(num_traversals));
  dependency_calls["rsp"] = -32;
  cstate =
      add_cstate_dependency(cstate, dependency_calls,
                            ether_addr_hash_cstate_contract()); /*For bridge*/

  return cstate;
}

std::map<std::string, std::set<int> >
map_erase_cstate_contract_0(std::vector<long> values) {

  long num_traversals = values[0];
  long num_collisions = values[1];

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}
#endif

/*  Double Map: */
std::map<std::string, std::set<int> >
dmap_allocate_cstate_contract_0(std::vector<long> values) {

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}

std::map<std::string, std::set<int> >
dmap_allocate_cstate_contract_1(std::vector<long> values) {

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}

std::map<std::string, std::set<int> >
dmap_get_a_cstate_contract_0(std::vector<long> values) {

  long num_traversals_a = values[0];
  long num_collisions_a = values[1];
  long success = values[2];
  assert(success);

  std::map<std::string, std::set<int> > cstate;
  cstate["rsp"] = { -8, -16, -24, -32, -40, -48, -56, -64 };
  cstate["rbx"] = { 80, 168, 56, 48, 40, 32, 64, 72 };
  std::map<std::string, int> dependency_calls;
  dependency_calls["rsp"] = -64;
  dependency_calls["rbx"] = 0;
  cstate =
      add_cstate_dependency(cstate, dependency_calls,
                            map_impl_get_cstate_contract(
                                success, num_traversals_a, num_collisions_a));
  dependency_calls["rsp"] = -32;
  cstate = add_cstate_dependency(cstate, dependency_calls,
                                 int_key_hash_cstate_contract()); /*For vignat*/
  return cstate;
}

std::map<std::string, std::set<int> >
dmap_get_a_cstate_contract_1(std::vector<long> values) {

  long num_traversals_a = values[0];
  long num_collisions_a = values[1];
  long success = values[2];
  assert(success == 0);

  std::map<std::string, std::set<int> > cstate;
  cstate["rsp"] = { -8, -16, -24, -32, -40, -48, -56, -64 };
  cstate["rbx"] = { 80, 168, 56, 48, 40, 32, 64, 72 };
  std::map<std::string, int> dependency_calls;
  dependency_calls["rsp"] = -64;
  dependency_calls["rbx"] = 0;
  cstate =
      add_cstate_dependency(cstate, dependency_calls,
                            map_impl_get_cstate_contract(
                                success, num_traversals_a, num_collisions_a));
  dependency_calls["rsp"] = -32;
  cstate = add_cstate_dependency(cstate, dependency_calls,
                                 int_key_hash_cstate_contract()); /*For vignat*/
  return cstate;
}

std::map<std::string, std::set<int> >
dmap_get_b_cstate_contract_0(std::vector<long> values) {

  long num_traversals_b = values[0];
  long num_collisions_b = values[1];
  long success = values[2];
  assert(success);

  std::map<std::string, std::set<int> > cstate;
  cstate["rsp"] = { -8, -16, -24, -32, -40, -48, -56, -64 };
  cstate["rbx"] = { 136, 168, 112, 104, 96, 88, 120, 128 };
  std::map<std::string, int> dependency_calls;
  dependency_calls["rsp"] = -64;
  dependency_calls["rbx"] = 0;
  cstate =
      add_cstate_dependency(cstate, dependency_calls,
                            map_impl_get_cstate_contract(
                                success, num_traversals_b, num_collisions_b));
  dependency_calls["rsp"] = -32;
  cstate = add_cstate_dependency(cstate, dependency_calls,
                                 ext_key_hash_cstate_contract()); /*For vignat*/
  return cstate;
}

std::map<std::string, std::set<int> >
dmap_get_b_cstate_contract_1(std::vector<long> values) {

  long num_traversals_b = values[0];
  long num_collisions_b = values[1];
  long success = values[2];
  assert(success == 0);

  std::map<std::string, std::set<int> > cstate;
  cstate["rsp"] = { -8, -16, -24, -32, -40, -48, -56, -64 };
  cstate["rbx"] = { 136, 168, 112, 104, 96, 88, 120, 128 };
  std::map<std::string, int> dependency_calls;
  dependency_calls["rsp"] = -64;
  dependency_calls["rbx"] = 0;
  cstate =
      add_cstate_dependency(cstate, dependency_calls,
                            map_impl_get_cstate_contract(
                                success, num_traversals_b, num_collisions_b));
  dependency_calls["rsp"] = -32;
  cstate = add_cstate_dependency(cstate, dependency_calls,
                                 ext_key_hash_cstate_contract()); /*For vignat*/
  return cstate;
}
std::map<std::string, std::set<int> >
dmap_get_value_cstate_contract_0(std::vector<long> values) {

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}

std::map<std::string, std::set<int> >
dmap_put_cstate_contract_0(std::vector<long> values) {

  long num_traversals_a = values[0];
  long num_traversals_b = values[1];

  std::map<std::string, std::set<int> > cstate;
  cstate["rsp"] = { -8, -16, -24, -32, -40, -48, -56, -64, -72, -80, -88, -96 };
  cstate["rbx"] = { 8,   168, 144, 80, 56, 48,  40,  32, 64,
                    136, 112, 104, 96, 88, 120, 160, 152 };
  std::map<std::string, int> dependency_calls;
  dependency_calls["rsp"] = -64;
  dependency_calls["rbx"] = 0;
  cstate = add_cstate_dependency(cstate, dependency_calls,
                                 flow_cpy_cstate_contract()); /*For vignat*/
  cstate =
      add_cstate_dependency(cstate, dependency_calls,
                            flow_extract_keys_cstate_contract()); /*For vignat*/
  cstate = add_cstate_dependency(cstate, dependency_calls,
                                 int_key_hash_cstate_contract()); /*For vignat*/
  cstate = add_cstate_dependency(cstate, dependency_calls,
                                 ext_key_hash_cstate_contract()); /*For vignat*/
  cstate =
      add_cstate_dependency(cstate, dependency_calls,
                            flow_pack_keys_cstate_contract()); /*For vignat*/

  dependency_calls["rsp"] = -96;
  cstate = add_cstate_dependency(
      cstate, dependency_calls,
      map_impl_put_cstate_contract(num_traversals_a)); /*For vignat*/
  cstate = add_cstate_dependency(
      cstate, dependency_calls,
      map_impl_put_cstate_contract(num_traversals_b)); /*For vignat*/
  return cstate;
}

std::map<std::string, std::set<int> >
dmap_erase_cstate_contract_0(std::vector<long> values) {

  long num_traversals_a = values[0];
  long num_traversals_b = values[1];
  long num_collisions_a = values[2];
  long num_collisions_b = values[3];

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}

/*  Expirator  */

std::map<std::string, std::set<int> >
expire_items_cstate_contract_0(std::vector<long> values) {

  long n_expired = values[4];

  std::map<std::string, std::set<int> > cstate;
  cstate["rsp"] = { -8, -16, -24, -32, -40, -48, -60, -72, -80 };
  std::map<std::string, int> dependency_calls;
  dependency_calls["rsp"] = -80;
  cstate = add_cstate_dependency(
      cstate, dependency_calls,
      dchain_expire_one_index_cstate_contract(0)); /*Final fail*/
  return cstate;
}

std::map<std::string, std::set<int> >
expire_items_single_map_cstate_contract_0(std::vector<long> values) {

  long n_expired = values[2];

  std::map<std::string, std::set<int> > cstate;
  cstate["rsp"] = { -8, -16, -24, -32, -40, -48, -56, -64 };
  std::map<std::string, int> dependency_calls;
  dependency_calls["rsp"] = -64;
  cstate = add_cstate_dependency(
      cstate, dependency_calls,
      dchain_expire_one_index_cstate_contract(0)); /*Final fail*/
  return cstate;
}

std::map<std::string, std::set<int> >
lb_find_preferred_available_backend_cstate_contract_0(
    std::vector<long> values) {

  long available_backends = values[0];
  std::map<std::string, std::set<int> > cstate;

  cstate["rsp"] = { -8, -16, -24, -32, -40, -48, -64, -72, -80, -96 };
  std::map<std::string, int> dependency_calls;
  dependency_calls["rsp"] = -96;
  cstate = add_cstate_dependency(cstate, dependency_calls,
                                 dchain_is_index_allocated_cstate_contract());
  cstate = add_cstate_dependency(cstate, dependency_calls,
                                 vector_borrow_cstate_contract_0(values));
  cstate = add_cstate_dependency(cstate, dependency_calls,
                                 vector_return_cstate_contract_0(values));
  return cstate;
}

std::map<std::string, std::set<int> >
flood_cstate_contract_0(std::vector<long> values) {

  std::map<std::string, std::set<int> > cstate;
  return cstate;
}
/* Functions called by workflow  */
bool check_metric(std::string metric) {
  if (std::find(supported_metrics.begin(), supported_metrics.end(), metric) !=
      supported_metrics.end()) {
    return true;
  }
  return false;
}
void contract_init() {
  supported_metrics = { "execution cycles", "instruction count",
                        "memory instructions" };
  fn_names = {
    "dchain_allocate",            "dchain_allocate_new_index",
    "dchain_rejuvenate_index",    "dchain2_allocate",
    "dchain2_allocate_new_index", "dchain2_rejuvenate_index",
    "map_allocate",               "map_get_1",
    "map_get_2",                  "map_put",
    "map_erase",                  "map2_allocate",
    "map2_get_1",                 "map2_put",
    "map2_erase",                 "dmap_allocate",
    "dmap_get_a",                 "dmap_get_b",
    "dmap_put",                   "dmap_erase",
    "dmap_get_value",             "expire_items",
    "expire_items_single_map",    "expire_items_single_map2",
    "vector_allocate",            "vector_borrow_full",
    "vector_borrow_half",         "vector_return_full",
    "vector_return_half",         "handle_packet_timestamp",
    "lpm_init",                   "lpm_lookup",
    "trace_reset_buffers",        "lb_find_preferred_available_backend",
    "flood", /*Hack*/
  };

  user_variables = {
    { "map_capacity", { "(w32 65536)" } },
    { "map2_capacity", { "(w32 65536)" } },
    { "dmap_occupancy",
      "(Sub w32 (ReadLSB w32 0 initial_dmap_capacity) (w32 1))" },
    { "dchain_out_of_space", "(ReadLSB w32 0 initial_dchain_out_of_space)" },
    { "dchain_out_of_space2", "(ReadLSB w32 0 initial_dchain_out_of_space2)" },
    { "is_dchain_allocated", "(ReadLSB w32 0 initial_is_dchain_allocated)" },
    { "is_dchain2_allocated", "(ReadLSB w32 0 initial_is_dchain2_allocated)" },
    { "dmap_allocation_succeeded",
      "(ReadLSB w32 0 initial_dmap_allocation_succeeded)" },
    { "dmap_capacity", "(ReadLSB w32 0 initial_dmap_capacity)" },
    { "dmap_has_this_key", "(ReadLSB w32 0 initial_dmap_has_this_key)" },
    { "map_occupancy",
      "(Sub w32 (ReadLSB w32 0 initial_map_capacity) (w32 1))" },
    { "map_backup_occupancy",
      "(Sub w32 (ReadLSB w32 0 initial_map_capacity) (w32 1))" },
    { "map2_occupancy",
      "(Sub w32 (ReadLSB w32 0 initial_map2_capacity) (w32 1))" },
    { "map2_backup_occupancy",
      "(Sub w32 (ReadLSB w32 0 initial_map2_capacity) (w32 1))" },
    { "Num_bucket_traversals", "(ReadLSB w32 0 initial_map_occupancy)" },
    { "Num_hash_collisions", "(ReadLSB w32 0 initial_Num_bucket_traversals)" },
    { "Num_bucket_traversals2", "(ReadLSB w32 0 initial_map2_occupancy)" },
    { "Num_hash_collisions2",
      "(ReadLSB w32 0 initial_Num_bucket_traversals2)" },
    { "Num_bucket_traversals_a", "(ReadLSB w32 0 initial_dmap_occupancy)" },
    { "Num_bucket_traversals_b", "(ReadLSB w32 0 initial_dmap_occupancy)" },
    { "Num_hash_collisions_a",
      "(ReadLSB w32 0 initial_Num_bucket_traversals_a)" },
    { "Num_hash_collisions_b",
      "(ReadLSB w32 0 initial_Num_bucket_traversals_b)" },
    { "expired_flows", "(ReadLSB w32 0 initial_map_occupancy)" },
    { "expired_flows2", "(ReadLSB w32 0 initial_map2_occupancy)" },
    { "map_has_this_key_a", "(ReadLSB w32 0 initial_map_has_this_key_a)" },
    { "map_has_this_key_b", "(ReadLSB w32 0 initial_map_has_this_key_b)" },
    { "map2_has_this_key_a", "(ReadLSB w32 0 initial_map2_has_this_key_a)" },
    { "timestamp_option", { "(w32 1)" } },
    { "lpm_stages", "(ReadLSB w32 0 initial_lpm_stages)" },
    { "available_backends", "(ReadLSB w32 0 initial_map2_capacity)" },
  };

  fn_user_variables = {
    { "dchain_allocate", {} }, { "dchain_allocate_new_index", {} },
    { "dchain_rejuvenate_index", {} }, { "dchain2_allocate", {} },
    { "dchain2_allocate_new_index", {} }, { "dchain2_rejuvenate_index", {} },
    { "map_allocate", {} },
    { "map_get_1", { "Num_bucket_traversals", "Num_hash_collisions" } },
    { "map_get_2", { "Num_bucket_traversals", "Num_hash_collisions" } },
    { "map_put", { "Num_bucket_traversals" } },
    { "map_erase", { "Num_bucket_traversals", "Num_hash_collisions" } },
    { "map2_allocate", {} },
    { "map2_get_1", { "Num_bucket_traversals2", "Num_hash_collisions2" } },
    { "map2_put", { "Num_bucket_traversals2" } },
    { "map2_erase", { "Num_bucket_traversals2", "Num_hash_collisions2" } },
    { "dmap_allocate", {} },
    { "dmap_get_a", { "Num_bucket_traversals_a", "Num_hash_collisions_a" } },
    { "dmap_get_b", { "Num_bucket_traversals_b", "Num_hash_collisions_b" } },
    { "dmap_get_value", {} },
    { "dmap_put", { "Num_bucket_traversals_a", "Num_bucket_traversals_b" } },
    { "dmap_erase", { "Num_bucket_traversals_a", "Num_bucket_traversals_b",
                      "Num_hash_collisions_a",   "Num_hash_collisions_b" } },
    { "expire_items", { "Num_bucket_traversals_a", "Num_bucket_traversals_b",
                        "Num_hash_collisions_a",   "Num_hash_collisions_b" } },
    { "expire_items_single_map",
      { "Num_bucket_traversals", "Num_hash_collisions" } },
    { "expire_items_single_map2",
      { "Num_bucket_traversals2", "Num_hash_collisions2" } },
    { "handle_packet_timestamp", { "timestamp_option" } }, { "lpm_lookup", {} },
    { "lb_find_preferred_available_backend", { "available_backends" } },
  };

  fn_optimization_variables = {
    { "dchain_allocate", { "is_dchain_allocated", "dchain_index_range" } },
    { "dchain2_allocate", { "is_dchain2_allocated", "dchain2_index_range" } },
    { "dchain_allocate_new_index", { "dchain_out_of_space" } },
    { "dchain2_allocate_new_index", { "dchain_out_of_space2" } },
    { "map_allocate", { "map_allocation_succeeded", "map_capacity" } },
    { "map_get_1", { "map_has_this_key_a", "map_occupancy" } },
    { "map_get_2", { "map_has_this_key_b", "map_occupancy" } },
    { "map_put", { "map_occupancy" } },
    { "map_erase", { "map_occupancy" } },
    { "map2_allocate", { "map2_allocation_succeeded", "map2_capacity" } },
    { "map2_get_1", { "map2_has_this_key_a", "map2_occupancy" } },
    { "map2_put", { "map2_occupancy" } },
    { "map2_erase", { "map2_occupancy" } },
    { "dmap_allocate", { "dmap_allocation_succeeded", "dmap_capacity" } },
    { "dmap_get_a", { "dmap_has_this_key" } },
    { "dmap_get_b", { "dmap_has_this_key" } },
    { "dmap_put", { "dmap_occupancy" } },
    { "dmap_erase", { "dmap_occupancy" } },
    { "dmap_get_value", {} },
    { "expire_items", { "expired_flows", "dmap_occupancy" } },
    { "expire_items_single_map", { "expired_flows", "map_occupancy" } },
    { "expire_items_single_map2", { "expired_flows2", "map2_occupancy" } },
    { "handle_packet_timestamp", { "timestamp_option" } },
    { "lpm_lookup", { "lpm_stages" } },
  };

  optimization_variables = {
    { "dmap_capacity", { "(ReadLSB w32 0 initial_dmap_capacity)" } },
    { "map_capacity", { "(ReadLSB w32 0 initial_map_capacity)" } },
    { "map2_capacity", { "(ReadLSB w32 0 initial_map2_capacity)" } },
    { "expired_flows",
      { "(w32 0)", "(ReadLSB w32 0 initial_dmap_occupancy)" } },
    { "expired_flows2",
      { "(w32 0)", "(ReadLSB w32 0 initial_map2_occupancy)" } },
    { "dchain_out_of_space", { "(w32 0)", "(w32 1)" } },
    { "dchain_out_of_space2", { "(w32 0)", "(w32 1)" } },
    { "dmap_allocation_succeeded", { "(w32 0)", "(w32 1)" } },
    { "map_allocation_succeeded", { "(w32 0)", "(w32 1)" } },
    { "map2_allocation_succeeded", { "(w32 0)", "(w32 1)" } },
    { "dmap_has_this_key", { "(w32 0)", "(w32 1)" } },
    { "map_has_this_key_a", { "(w32 0)", "(w32 1)" } },
    { "map_has_this_key_b", { "(w32 0)", "(w32 1)" } },
    { "map2_has_this_key_a", { "(w32 0)", "(w32 1)" } },
    { "is_dchain_allocated", { "(w32 0)", "(w32 1)" } },
    { "is_dchain2_allocated", { "(w32 0)", "(w32 1)" } },
    { "timestamp_option", { "(w32 0)", "(w32 1)" } },
    { "lpm_stages", { "(w32 1)", "(w32 2)" } },
  };

  fn_variables.insert(fn_user_variables.begin(), fn_user_variables.end());
  for (std::map<std::string, std::vector<std::string> >::iterator i =
           fn_variables.begin();
       i != fn_variables.end(); ++i) {
    i->second.insert(i->second.end(),
                     fn_optimization_variables[i->first].begin(),
                     fn_optimization_variables[i->first].end());
  }
  constraints = {
    //{"fn_name",{ {0,{"constraint1","constraint2" }} , {1,{"constraint1",
    //"constraint2"}}  }},
    { "dchain_allocate",
      { { 0, "(Eq false (Eq 0 (ReadLSB w32 0 current_is_dchain_allocated)))" },
        { 1, "(Eq 0 (ReadLSB w32 0 current_is_dchain_allocated))" } } },
    { "dchain_allocate_new_index",
      { { 0, "(Eq 0 (ReadLSB w32 0 current_dchain_out_of_space))" },
        { 1,
          "(Eq false (Eq 0 (ReadLSB w32 0 current_dchain_out_of_space)))" } } },
    { "dchain_rejuvenate_index", { { 0, "true" } } },
    { "dchain2_allocate",
      { { 0, "(Eq false (Eq 0 (ReadLSB w32 0 current_is_dchain2_allocated)))" },
        { 1, "(Eq 0 (ReadLSB w32 0 current_is_dchain2_allocated))" } } },
    { "dchain2_allocate_new_index",
      { { 0, "(Eq 0 (ReadLSB w32 0 current_dchain_out_of_space2))" },
        { 1, "(Eq false (Eq 0 (ReadLSB w32 0 "
             "current_dchain_out_of_space2)))" } } },
    { "dchain2_rejuvenate_index", { { 0, "true" } } },
    { "map_allocate",
      { { 0, "(Eq false (Eq 0 (ReadLSB w32 0 "
             "current_map_allocation_succeeded)))" },
        { 1, "(Eq 0 (ReadLSB w32 0 current_map_allocation_succeeded))" } } },
    { "map_get_1",
      { { 0, "(Eq false (Eq 0 (ReadLSB w32 0 current_map_has_this_key_a)))" },
        { 1, "(Eq 0 (ReadLSB w32 0 current_map_has_this_key_a))" } } },
    { "map_get_2",
      { { 0, "(Eq false (Eq 0 (ReadLSB w32 0 current_map_has_this_key_b)))" },
        { 1, "(Eq 0 (ReadLSB w32 0 current_map_has_this_key_b))" } } },
    { "map_put", { { 0, "true" } } }, { "map_erase", { { 0, "true" } } },
    { "map2_allocate",
      { { 0, "(Eq false (Eq 0 (ReadLSB w32 0 "
             "current_map2_allocation_succeeded)))" },
        { 1, "(Eq 0 (ReadLSB w32 0 current_map2_allocation_succeeded))" } } },
    { "map2_get_1",
      { { 0, "(Eq false (Eq 0 (ReadLSB w32 0 current_map2_has_this_key_a)))" },
        { 1, "(Eq 0 (ReadLSB w32 0 current_map2_has_this_key_a))" } } },
    { "map2_put", { { 0, "true" } } }, { "map2_erase", { { 0, "true" } } },
    { "dmap_allocate",
      { { 0, "(Eq false (Eq 0 (ReadLSB w32 0 "
             "current_dmap_allocation_succeeded)))" },
        { 1, "(Eq 0 (ReadLSB w32 0 current_dmap_allocation_succeeded))" } } },
    { "dmap_get_a",
      { { 0, "(Eq false (Eq 0 (ReadLSB w32 0 current_dmap_has_this_key)))" },
        { 1, "(Eq 0 (ReadLSB w32 0 current_dmap_has_this_key))" } } },
    { "dmap_get_b",
      { { 0, "(Eq false (Eq 0 (ReadLSB w32 0 current_dmap_has_this_key)))" },
        { 1, "(Eq 0 (ReadLSB w32 0 current_dmap_has_this_key))" } } },
    { "dmap_put", { { 0, "true" } } }, { "dmap_get_value", { { 0, "true" } } },
    { "dmap_erase", { { 0, "true" } } }, { "expire_items", { { 0, "true" } } },
    { "expire_items_single_map", { { 0, "true" } } },
    { "expire_items_single_map2", { { 0, "true" } } },
    { "vector_allocate", { { 0, "true" } } },
    { "vector_borrow_full", { { 0, "true" } } },
    { "vector_borrow_half", { { 0, "true" } } },
    { "vector_return_full", { { 0, "true" } } },
    { "vector_return_half", { { 0, "true" } } },
    { "handle_packet_timestamp",
      { { 0, "(Eq 1 (ReadLSB w32 0 current_timestamp_option))" } } },
    { "lpm_init", { { 0, "true" } } }, { "lpm_lookup", { { 0, "true" } } },
    { "trace_reset_buffers", { { 0, "true" } } },
    { "lb_find_preferred_available_backend", { { 0, "true" } } },
    { "flood", { { 0, "true" } } },
  };

  perf_fn_ptrs = {
    { "dchain_allocate", { { 0, &dchain_allocate_contract_0 },
                           { 1, &dchain_allocate_contract_1 } } },
    { "dchain_allocate_new_index",
      { { 0, &dchain_allocate_new_index_contract_0 },
        { 1, &dchain_allocate_new_index_contract_1 } } },
    { "dchain_rejuvenate_index",
      { { 0, &dchain_rejuvenate_index_contract_0 } } },
    { "dchain2_allocate", { { 0, &dchain_allocate_contract_0 },
                            { 1, &dchain_allocate_contract_1 } } },
    { "dchain2_allocate_new_index",
      { { 0, &dchain_allocate_new_index_contract_0 },
        { 1, &dchain_allocate_new_index_contract_1 } } },
    { "dchain2_rejuvenate_index",
      { { 0, &dchain_rejuvenate_index_contract_0 } } },
    { "map_allocate",
      { { 0, &map_allocate_contract_0 }, { 1, map_allocate_contract_1 } } },
    { "map_get_1", { { 0, &map_get_contract_0 }, { 1, map_get_contract_1 } } },
    { "map_get_2", { { 0, &map_get_contract_0 }, { 1, map_get_contract_1 } } },
    { "map_put", { { 0, &map_put_contract_0 } } },
    { "map_erase", { { 0, &map_erase_contract_0 } } },
    { "map2_allocate",
      { { 0, &map_allocate_contract_0 }, { 1, map_allocate_contract_1 } } },
    { "map2_get_1", { { 0, &map_get_contract_0 }, { 1, map_get_contract_1 } } },
    { "map2_put", { { 0, &map_put_contract_0 } } },
    { "map2_erase", { { 0, &map_erase_contract_0 } } },
    { "dmap_allocate",
      { { 0, &dmap_allocate_contract_0 }, { 1, dmap_allocate_contract_1 } } },
    { "dmap_get_a",
      { { 0, &dmap_get_a_contract_0 }, { 1, dmap_get_a_contract_1 } } },
    { "dmap_get_b",
      { { 0, &dmap_get_b_contract_0 }, { 1, dmap_get_b_contract_1 } } },
    { "dmap_put", { { 0, &dmap_put_contract_0 } } },
    { "dmap_erase", { { 0, &dmap_erase_contract_0 } } },
    { "dmap_get_value", { { 0, &dmap_get_value_contract_0 } } },
    { "expire_items", { { 0, &expire_items_contract_0 } } },
    { "expire_items_single_map",
      { { 0, &expire_items_single_map_contract_0 } } },
    { "expire_items_single_map2",
      { { 0, &expire_items_single_map_contract_0 } } },
    { "vector_allocate", { { 0, &vector_allocate_contract_0 } } },
    { "vector_borrow_half", { { 0, &vector_borrow_contract_0 } } },
    { "vector_borrow_full", { { 0, &vector_borrow_contract_0 } } },
    { "vector_return_half", { { 0, &vector_return_contract_0 } } },
    { "vector_return_full", { { 0, &vector_return_contract_0 } } },
    { "lpm_init", { { 0, &lpm_init_contract_0 } } },
    { "lpm_lookup", { { 0, &lpm_lookup_contract_0 } } },
    { "trace_reset_buffers", { { 0, &trace_reset_buffers_contract_0 } } },
    { "handle_packet_timestamp",
      { { 0, &handle_packet_timestamp_contract_0 } } },
    { "lb_find_preferred_available_backend",
      { { 0, &lb_find_preferred_available_backend_contract_0 } } },
    { "flood", { { 0, &flood_contract_0 } } },
  };

  cstate_fn_ptrs = {
    { "dchain_allocate", { { 0, &dchain_allocate_cstate_contract_0 },
                           { 1, &dchain_allocate_cstate_contract_1 } } },
    { "dchain_allocate_new_index",
      { { 0, &dchain_allocate_new_index_cstate_contract_0 },
        { 1, &dchain_allocate_new_index_cstate_contract_1 } } },
    { "dchain_rejuvenate_index",
      { { 0, &dchain_rejuvenate_index_cstate_contract_0 } } },
    { "dchain2_allocate", { { 0, &dchain_allocate_cstate_contract_0 },
                            { 1, &dchain_allocate_cstate_contract_1 } } },
    { "dchain2_allocate_new_index",
      { { 0, &dchain_allocate_new_index_cstate_contract_0 },
        { 1, &dchain_allocate_new_index_cstate_contract_1 } } },
    { "dchain2_rejuvenate_index",
      { { 0, &dchain_rejuvenate_index_cstate_contract_0 } } },
    { "map_allocate", { { 0, &map_allocate_cstate_contract_0 },
                        { 1, map_allocate_cstate_contract_1 } } },
    { "map_get_1",
      { { 0, &map_get_cstate_contract_0 }, { 1, map_get_cstate_contract_1 } } },
    { "map_get_2",
      { { 0, &map_get_cstate_contract_0 }, { 1, map_get_cstate_contract_1 } } },
    { "map_put", { { 0, &map_put_cstate_contract_0 } } },
    { "map_erase", { { 0, &map_erase_cstate_contract_0 } } },
    { "map2_allocate", { { 0, &map_allocate_cstate_contract_0 },
                         { 1, map_allocate_cstate_contract_1 } } },
    { "map2_get_1",
      { { 0, &map_get_cstate_contract_0 }, { 1, map_get_cstate_contract_1 } } },
    { "map2_put", { { 0, &map_put_cstate_contract_0 } } },
    { "map2_erase", { { 0, &map_erase_cstate_contract_0 } } },
    { "dmap_allocate", { { 0, &dmap_allocate_cstate_contract_0 },
                         { 1, dmap_allocate_cstate_contract_1 } } },
    { "dmap_get_a", { { 0, &dmap_get_a_cstate_contract_0 },
                      { 1, dmap_get_a_cstate_contract_1 } } },
    { "dmap_get_b", { { 0, &dmap_get_b_cstate_contract_0 },
                      { 1, dmap_get_b_cstate_contract_1 } } },
    { "dmap_put", { { 0, &dmap_put_cstate_contract_0 } } },
    { "dmap_erase", { { 0, &dmap_erase_cstate_contract_0 } } },
    { "dmap_get_value", { { 0, &dmap_get_value_cstate_contract_0 } } },
    { "expire_items", { { 0, &expire_items_cstate_contract_0 } } },
    { "expire_items_single_map",
      { { 0, &expire_items_single_map_cstate_contract_0 } } },
    { "expire_items_single_map2",
      { { 0, &expire_items_single_map_cstate_contract_0 } } },
    { "vector_allocate", { { 0, &vector_allocate_cstate_contract_0 } } },
    { "vector_borrow_half", { { 0, &vector_borrow_cstate_contract_0 } } },
    { "vector_borrow_full", { { 0, &vector_borrow_cstate_contract_0 } } },
    { "vector_return_half", { { 0, &vector_return_cstate_contract_0 } } },
    { "vector_return_full", { { 0, &vector_return_cstate_contract_0 } } },
    { "lpm_init", { { 0, &lpm_init_cstate_contract_0 } } },
    { "lpm_lookup", { { 0, &lpm_lookup_cstate_contract_0 } } },
    { "trace_reset_buffers",
      { { 0, &trace_reset_buffers_cstate_contract_0 } } },
    { "handle_packet_timestamp",
      { { 0, &handle_packet_timestamp_cstate_contract_0 } } },
    { "lb_find_preferred_available_backend",
      { { 0, &lb_find_preferred_available_backend_cstate_contract_0 } } },
    { "flood", { { 0, &flood_cstate_contract_0 } } },
  };

  std::cerr << "Loading Performance Contracts." << std::endl;
}
/* **************************************** */
std::map<std::string, std::string> contract_get_user_variables() {
  return user_variables;
}
/* **************************************** */
std::set<std::string> contract_get_contracts() {
  return std::set<std::string>(fn_names.begin(), fn_names.end());
}
/* **************************************** */
std::set<std::string> contract_get_metrics() {
  return std::set<std::string>(supported_metrics.begin(),
                               supported_metrics.end());
}
/* **************************************** */
bool contract_has_contract(std::string function_name) {
  if (std::find(fn_names.begin(), fn_names.end(), function_name) !=
      fn_names.end()) {
    return true;
  }
  return false;
}
/* **************************************** */
std::map<std::string, std::set<std::string> >
contract_get_optimization_variables() {
  return optimization_variables;
}
/* **************************************** */
std::set<std::string> contract_get_contract_symbols(std::string function_name) {
  return std::set<std::string>(fn_variables[function_name].begin(),
                               fn_variables[function_name].end());
}
/* **************************************** */
std::set<std::string> contract_get_symbols() {
  return {

    /* Map symbols */
    "array map_allocation_succeeded[4] : w32 -> w8 = symbolic",
    "array current_map_allocation_succeeded[4] : w32 -> w8 = symbolic",
    "array initial_map_allocation_succeeded[4] : w32 -> w8 = symbolic",
    "array map_capacity[4] : w32 -> w8 = symbolic",
    "array current_map_capacity[4] : w32 -> w8 = symbolic",
    "array initial_map_capacity[4] : w32 -> w8 = symbolic",
    "array map_occupancy[4] : w32 -> w8 = symbolic",
    "array current_map_occupancy[4] : w32 -> w8 = symbolic",
    "array initial_map_occupancy[4] : w32 -> w8 = symbolic",
    "array map_backup_occupancy[4] : w32 -> w8 = symbolic", // Kludge
    "array current_map_backup_occupancy[4] : w32 -> w8 = symbolic",
    "array initial_map_backup_occupancy[4] : w32 -> w8 = symbolic",
    "array Num_bucket_traversals[4] : w32 -> w8 = symbolic",
    "array current_Num_bucket_traversals[4] : w32 -> w8 = symbolic",
    "array initial_Num_bucket_traversals[4] : w32 -> w8 = symbolic",
    "array Num_hash_collisions[4] : w32 -> w8 = symbolic",
    "array current_Num_hash_collisions[4] : w32 -> w8 = symbolic",
    "array initial_Num_hash_collisions[4] : w32 -> w8 = symbolic",
    "array map_has_this_key_a[4] : w32 -> w8 = symbolic",
    "array current_map_has_this_key_a[4] : w32 -> w8 = symbolic",
    "array initial_map_has_this_key_a[4] : w32 -> w8 = symbolic",
    "array map_has_this_key_b[4] : w32 -> w8 = symbolic", // Kludge
    "array current_map_has_this_key_b[4] : w32 -> w8 = symbolic",
    "array initial_map_has_this_key_b[4] : w32 -> w8 = symbolic",

    /* Map2 symbols */
    "array map2_allocation_succeeded[4] : w32 -> w8 = symbolic",
    "array current_map2_allocation_succeeded[4] : w32 -> w8 = symbolic",
    "array initial_map2_allocation_succeeded[4] : w32 -> w8 = symbolic",
    "array map2_capacity[4] : w32 -> w8 = symbolic",
    "array current_map2_capacity[4] : w32 -> w8 = symbolic",
    "array initial_map2_capacity[4] : w32 -> w8 = symbolic",
    "array map2_occupancy[4] : w32 -> w8 = symbolic",
    "array current_map2_occupancy[4] : w32 -> w8 = symbolic",
    "array initial_map2_occupancy[4] : w32 -> w8 = symbolic",
    "array map2_backup_occupancy[4] : w32 -> w8 = symbolic", // Kludge
    "array current_map2_backup_occupancy[4] : w32 -> w8 = symbolic",
    "array initial_map2_backup_occupancy[4] : w32 -> w8 = symbolic",
    "array Num_bucket_traversals2[4] : w32 -> w8 = symbolic",
    "array current_Num_bucket_traversals2[4] : w32 -> w8 = symbolic",
    "array initial_Num_bucket_traversals2[4] : w32 -> w8 = symbolic",
    "array Num_hash_collisions2[4] : w32 -> w8 = symbolic",
    "array current_Num_hash_collisions2[4] : w32 -> w8 = symbolic",
    "array initial_Num_hash_collisions2[4] : w32 -> w8 = symbolic",
    "array map2_has_this_key_a[4] : w32 -> w8 = symbolic",
    "array current_map2_has_this_key_a[4] : w32 -> w8 = symbolic",
    "array initial_map2_has_this_key_a[4] : w32 -> w8 = symbolic",

    /* Double Map symbols */
    "array dmap_allocation_succeeded[4] : w32 -> w8 = symbolic",
    "array current_dmap_allocation_succeeded[4] : w32 -> w8 = symbolic",
    "array initial_dmap_allocation_succeeded[4] : w32 -> w8 = symbolic",
    "array dmap_capacity[4] : w32 -> w8 = symbolic",
    "array current_dmap_capacity[4] : w32 -> w8 = symbolic",
    "array initial_dmap_capacity[4] : w32 -> w8 = symbolic",
    "array dmap_has_this_key[4] : w32 -> w8 = symbolic",
    "array current_dmap_has_this_key[4] : w32 -> w8 = symbolic",
    "array initial_dmap_has_this_key[4] : w32 -> w8 = symbolic",
    "array dmap_occupancy[4] : w32 -> w8 = symbolic",
    "array current_dmap_occupancy[4] : w32 -> w8 = symbolic",
    "array initial_dmap_occupancy[4] : w32 -> w8 = symbolic",
    "array Num_bucket_traversals_a[4] : w32 -> w8 = symbolic",
    "array current_Num_bucket_traversals_a[4] : w32 -> w8 = symbolic",
    "array initial_Num_bucket_traversals_a[4] : w32 -> w8 = symbolic",
    "array Num_bucket_traversals_b[4] : w32 -> w8 = symbolic",
    "array current_Num_bucket_traversals_b[4] : w32 -> w8 = symbolic",
    "array initial_Num_bucket_traversals_b[4] : w32 -> w8 = symbolic",
    "array Num_hash_collisions_a[4] : w32 -> w8 = symbolic",
    "array current_Num_hash_collisions_a[4] : w32 -> w8 = symbolic",
    "array initial_Num_hash_collisions_a[4] : w32 -> w8 = symbolic",
    "array Num_hash_collisions_b[4] : w32 -> w8 = symbolic",
    "array current_Num_hash_collisions_b[4] : w32 -> w8 = symbolic",
    "array initial_Num_hash_collisions_b[4] : w32 -> w8 = symbolic",

    /* Double Chain symbols */
    "array is_dchain_allocated[4] : w32 -> w8 = symbolic",
    "array current_is_dchain_allocated[4] : w32 -> w8 = symbolic",
    "array initial_is_dchain_allocated[4] : w32 -> w8 = symbolic",
    "array dchain_index_range[4] : w32 -> w8 = symbolic",
    "array current_dchain_index_range[4] : w32 -> w8 = symbolic",
    "array initial_dchain_index_range[4] : w32 -> w8 = symbolic",
    "array dchain_out_of_space[4] : w32 -> w8 = symbolic",
    "array current_dchain_out_of_space[4] : w32 -> w8 = symbolic",
    "array initial_dchain_out_of_space[4] : w32 -> w8 = symbolic",

    /* Double Chain2 symbols */
    "array is_dchain2_allocated[4] : w32 -> w8 = symbolic",
    "array current_is_dchain2_allocated[4] : w32 -> w8 = symbolic",
    "array initial_is_dchain2_allocated[4] : w32 -> w8 = symbolic",
    "array dchain2_index_range[4] : w32 -> w8 = symbolic",
    "array current_dchain2_index_range[4] : w32 -> w8 = symbolic",
    "array initial_dchain2_index_range[4] : w32 -> w8 = symbolic",
    "array dchain_out_of_space2[4] : w32 -> w8 = symbolic",
    "array current_dchain_out_of_space2[4] : w32 -> w8 = symbolic",
    "array initial_dchain_out_of_space2[4] : w32 -> w8 = symbolic",

    /* Expirator symbols */
    "array expired_flows[4] : w32 -> w8 = symbolic",
    "array current_expired_flows[4] : w32 -> w8 = symbolic",
    "array initial_expired_flows[4] : w32 -> w8 = symbolic",

    /* Expirator symbols */
    "array expired_flows2[4] : w32 -> w8 = symbolic",
    "array current_expired_flows2[4] : w32 -> w8 = symbolic",
    "array initial_expired_flows2[4] : w32 -> w8 = symbolic",

    /* CHT symbols */
    "array available_backends[4] : w32 -> w8 = symbolic",
    "array current_available_backends[4] : w32 -> w8 = symbolic",
    "array initial_available_backends[4] : w32 -> w8 = symbolic",

    /* Vector symbols */
    "array borrowed_cell[6] : w32 -> w8 = symbolic", // Legacy from Vigor,
                                                     // not
                                                     // required by Bolt
    "array current_borrowed_cell[6] : w32 -> w8 = symbolic",
    "array initial_borrowed_cell[6] : w32 -> w8 = symbolic",

    /* Other symbols */
    "array incoming_package[4] : w32 -> w8 = symbolic",
    "array current_incoming_package[4] : w32 -> w8 = symbolic",
    "array initial_incoming_package[4] : w32 -> w8 = symbolic",
    "array user_buf_addr[4] : w32 -> w8 = symbolic",
    "array current_user_buf_addr[4] : w32 -> w8 = symbolic",
    "array initial_user_buf_addr[4] : w32 -> w8 = symbolic",
    "array mbuf[4] : w32 -> w8 = symbolic",
    "array current_mbuf[4] : w32 -> w8 = symbolic",
    "array initial_mbuf[4] : w32 -> w8 = symbolic",
    "array lpm_stages[4] : w32 -> w8 = symbolic",
    "array current_lpm_stages[4] : w32 -> w8 = symbolic",
    "array initial_lpm_stages[4] : w32 -> w8 = symbolic",
    "array timestamp_option[4] : w32 -> w8 = symbolic",
    "array current_timestamp_option[4] : w32 -> w8 = symbolic",
    "array initial_timestamp_option[4] : w32 -> w8 = symbolic",
  };
}
/* **************************************** */
int contract_num_sub_contracts(std::string function_name) {
  return constraints[function_name].size();
}
/* **************************************** */
std::string contract_get_subcontract_constraints(std::string function_name,
                                                 int sub_contract_idx) {
  return constraints[function_name][sub_contract_idx];
}
/* **************************************** */
long
contract_get_sub_contract_performance(std::string function_name,
                                      int sub_contract_idx, std::string metric,
                                      std::map<std::string, long> variables) {
  if (!(check_metric(metric)))
    return -1;

  relevant_vars = fn_variables[function_name];
  relevant_vals.clear();
  for (std::vector<std::string>::iterator i = relevant_vars.begin();
       i != relevant_vars.end(); ++i) {
    if (variables.find(*i) == variables.end()) {
      std::cerr << "Required variable " << *i << " not sent for function "
                << function_name << std::endl;
      return -1;
    } else
      relevant_vals.insert(relevant_vals.end(), variables[*i]);
  }
  perf_calc_fn_ptr fn_ptr;
  fn_ptr = perf_fn_ptrs[function_name][sub_contract_idx];
  assert(fn_ptr);
  long perf = fn_ptr(metric, relevant_vals);
  if (metric == "memory instructions" && perf > 0) {
    perf = perf - 1;
  }
  return perf;
}

/* **************************************** */
std::map<std::string, std::set<int> >
contract_get_concrete_state(std::string function_name, int sub_contract_idx,
                            std::map<std::string, long> variables) {

  relevant_vars = fn_variables[function_name];
  relevant_vals.clear();
  for (std::vector<std::string>::iterator i = relevant_vars.begin();
       i != relevant_vars.end(); ++i) {
    if (variables.find(*i) == variables.end()) {
      std::cerr << "Required variable " << *i << " not sent for function "
                << function_name << std::endl;
      assert(0);
    } else
      relevant_vals.insert(relevant_vals.end(), variables[*i]);
  }

  cstate_fn_ptr fn_ptr;
  fn_ptr = cstate_fn_ptrs[function_name][sub_contract_idx];
  assert(fn_ptr && "Function pointer not found for cstate call\n");

  std::map<std::string, std::set<int> > cstate = fn_ptr(relevant_vals);
  return cstate;
}

int main() {}
