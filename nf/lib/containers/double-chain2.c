#include <stdlib.h>
#include <string.h>

#include "double-chain.h"
#include "double-chain-impl.h"


struct DoubleChain {
  struct dchain_cell* cells;
  time_t *timestamps;
};




int dchain2_allocate(int index_range, struct DoubleChain** chain_out)
{

  struct DoubleChain* old_chain_out = *chain_out;
  struct DoubleChain* chain_alloc = malloc(sizeof(struct DoubleChain));
  if (chain_alloc == NULL) return 0;
  *chain_out = (struct DoubleChain*) chain_alloc;

  struct dchain_cell* cells_alloc =
    malloc(sizeof (struct dchain_cell)*(index_range + DCHAIN_RESERVED));
  if (cells_alloc == NULL) {
    free(chain_alloc);
    *chain_out = old_chain_out;
    return 0;
  }
  (*chain_out)->cells = cells_alloc;

  time_t* timestamps_alloc = malloc(sizeof(time_t)*(index_range));
  if (timestamps_alloc == NULL) {
    free((void*)cells_alloc);
    free(chain_alloc);
    *chain_out = old_chain_out;
    return 0;
  }
  (*chain_out)->timestamps = timestamps_alloc;

  dchain_impl_init((*chain_out)->cells, index_range);
  return 1;
}



int dchain2_allocate_new_index(struct DoubleChain* chain,
                              int *index_out, time_t time)
{
  int ret = dchain_impl_allocate_new_index(chain->cells, index_out);
  if (ret) {
    chain->timestamps[*index_out] = time;
  } else {
  }
  return ret;
}


int dchain2_rejuvenate_index(struct DoubleChain* chain,
                            int index, time_t time)
{
  int ret = dchain_impl_rejuvenate_index(chain->cells, index);
  if (ret) {
    chain->timestamps[index] = time;
  } else {
  }
  return ret;
}

int dchain2_expire_one_index(struct DoubleChain* chain,
                            int* index_out, time_t time)
{
  int has_ind = dchain_impl_get_oldest_index(chain->cells, index_out);
  if (has_ind) {
    if (chain->timestamps[*index_out] < time) {
      int rez = dchain_impl_free_index(chain->cells, *index_out);
      return rez;
    }
  }
  return 0;
}


int dchain2_is_index_allocated(struct DoubleChain* chain, int index)
{
  return dchain_impl_is_index_allocated(chain->cells, index);
}


