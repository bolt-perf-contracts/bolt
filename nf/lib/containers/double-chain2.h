#ifndef _DOUBLE_CHAIN2_H_INCLUDED_
#define _DOUBLE_CHAIN2_H_INCLUDED_

#include <stdint.h>

#include "lib/nf_time.h"

//@ #include <listex.gh>
//@ #include "stdex.gh"

struct DoubleChain;
/* Makes sure the allocator structur fits into memory, and particularly into
   32 bit address space. @*/
#define IRANG_LIMIT (1048576)

// kinda hacky, but makes the proof independent of time_t... sort of
#define malloc_block_time malloc_block_llongs
#define time_integer llong_integer
#define times llongs


/**
   Allocate memory and initialize a new double chain allocator. The produced
   allocator will operate on indexes [0-index).

   @param index_range - the limit on the number of allocated indexes.
   @param chain_out - an output pointer that will hold the pointer to the newly
                      allocated allocator in the case of success.
   @returns 0 if the allocation failed, and 1 if the allocation is successful.
*/
int dchain2_allocate(int index_range, struct DoubleChain** chain_out);

/**
   Allocate a fresh index. If there is an unused, or expired index in the range,
   allocate it.

   @param chain - pointer to the allocator.
   @param index_out - output pointer to the newly allocated index.
   @param time - current time. Allocator will note this for the new index.
   @returns 0 if there is no space, and 1 if the allocation is successful.
 */
int dchain2_allocate_new_index(struct DoubleChain* chain,
                              int* index_out, time_t time);

/**
   Update the index timestamp. Needed to keep the index from expiration.

   @param chain - pointer to the allocator.
   @param index - the index to rejuvenate.
   @param time - the current time, it will replace the old timestamp.
   @returns 1 if the timestamp was updated, and 0 if the index is not tagged as
            allocated.
 */
int dchain2_rejuvenate_index(struct DoubleChain* chain,
                            int index, time_t time);

/**
   Make space in the allocator by expiring the least recently used index.

   @param chain - pointer to the allocator.
   @param index_out - output pointer to the expired index.
   @param time - the time border, separating expired indexes from non-expired
                 ones.
   @returns 1 if the oldest index is older then current time and is expired,
   0 otherwise.
 */
int dchain2_expire_one_index(struct DoubleChain* chain,
                            int* index_out, time_t time);

int dchain2_is_index_allocated(struct DoubleChain* chain, int index);

#endif //_DOUBLE_CHAIN2_H_INCLUDED_
