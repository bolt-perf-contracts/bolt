#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <klee/klee.h>
#include "lib/containers/double-chain.h"

// TODO: double check that this model is enough for the NAT scenario

#define ALLOW(chain) klee_allow_access((chain), sizeof(struct DoubleChain))
#define DENY(chain) klee_forbid_access((chain), sizeof(struct DoubleChain), "allocated_chain_do_not_dereference")

struct DoubleChain {
  int dchain_out_of_space;
  int new_index;
  int is_index_allocated;
};

__attribute__((noinline))
int dchain2_allocate(int index_range, struct DoubleChain** chain_out) {
    klee_trace_ret();
    klee_trace_param_i32(index_range, "index_range");
    klee_trace_param_ptr_directed(chain_out, sizeof(struct DoubleChain*), "chain_out", TD_OUT);

    // TODO not needed if malloc can fail
    int is_dchain2_allocated = klee_int("is_dchain2_allocated");
    *chain_out = malloc(sizeof(struct DoubleChain));

    klee_possibly_havoc(*chain_out, sizeof(struct DoubleChain), "chain_out");
    klee_trace_extra_ptr(&is_dchain2_allocated, sizeof(is_dchain2_allocated),
                         "is_dchain2_allocated", "type", TD_BOTH);
    klee_trace_extra_ptr(&index_range, sizeof(index_range),
                         "dchain2_index_range", "type", TD_BOTH);

    if (is_dchain2_allocated && *chain_out != NULL) {
      memset(*chain_out, 0, sizeof(struct DoubleChain));
      (*chain_out)->new_index = klee_int("new_index");
      klee_assume(0 <= (*chain_out)->new_index);
      klee_assume((*chain_out)->new_index < index_range);
      (*chain_out)->is_index_allocated = 0;
      (*chain_out)->dchain_out_of_space = klee_int("dchain_out_of_space2");
      DENY(*chain_out);
      return 1;
    } else {
      return 0;
    }
}

__attribute__((noinline))
int dchain2_allocate_new_index(struct DoubleChain* chain, int *index_out,
                              time_t time) {
    klee_trace_ret();
    //Deliberately trace this pointer as an integer to avoid
    // dereference.
    klee_trace_param_u64((uint64_t)chain, "chain");
    klee_trace_param_ptr(index_out, sizeof(int), "index_out");
    klee_trace_param_i32(time, "time");

    ALLOW(chain);
    klee_trace_extra_ptr(&chain->dchain_out_of_space, sizeof(chain->dchain_out_of_space),
                         "dchain_out_of_space2", "type", TD_BOTH);
    if (chain->dchain_out_of_space) {
      DENY(chain);
      return 0;
    }

    klee_assert(!(chain->is_index_allocated));
    *index_out = chain->new_index;
    chain->is_index_allocated = 1;
    DENY(chain);
    return 1;
}

__attribute__((noinline))
int dchain2_rejuvenate_index(struct DoubleChain* chain, int index,
                            time_t time) {
    klee_trace_ret();
    //Deliberately trace this pointer as an integer to avoid
    // dereference.
    klee_trace_param_u64((uint64_t)chain, "chain");
    klee_trace_param_i32(index, "index");
    klee_trace_param_i32(time, "time");

    klee_assert(chain != NULL);
    // TODO: Check if it is legible for rejuivenation?
    return 1;
}

__attribute__((noinline))
int dchain2_expire_one_index(struct DoubleChain* chain,
                            int* index_out, time_t time) {
  klee_trace_ret();
  //Deliberately trace this pointer as an integer to avoid
  // dereference.
  klee_trace_param_u64((uint64_t)chain, "chain");
  klee_trace_param_ptr(index_out, sizeof(int), "index_out");
  klee_trace_param_i32(time, "time");

  klee_assert(chain != NULL);
  klee_assert(0 && "not supported in this model");
  return 0;
}

__attribute__((noinline))
int dchain2_is_index_allocated(struct DoubleChain* chain, int index) {
  klee_trace_ret();
  //Deliberately trace this pointer as an integer to avoid dereference
  klee_trace_param_u64((uint64_t)chain, "chain");
  klee_trace_param_i32(index, "index");

  klee_assert(chain != NULL);
  return klee_int("dchain2_is_index_allocated");
}

void dchain2_make_space(struct DoubleChain* chain) {
  //Do not trace. this function is internal for the Expirator model.
  ALLOW(chain);
  klee_assert(chain->is_index_allocated == 0);
  //Do not trace internal stub control functions.
  chain->dchain_out_of_space = 0;
  DENY(chain);
}

void dchain2_reset(struct DoubleChain* chain, int index_range) {
  //Do not trace. This function is an internal knob of the model.
  ALLOW(chain);
  chain->new_index = klee_int("new_index");
  klee_assume(0 <= chain->new_index);
  klee_assume(chain->new_index < index_range);
  chain->is_index_allocated = 0;
  chain->dchain_out_of_space = klee_int("dchain_out_of_space2");
  DENY(chain);
}
