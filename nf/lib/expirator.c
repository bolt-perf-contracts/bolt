#include <assert.h>
#include "lib/containers/double-chain.h"
#include "lib/coherence.h"
#include "lib/expirator.h"

#ifdef DUMP_PERF_VARS
#include "lib/nf_log.h"
#endif

/*@
  lemma void expire_0_indexes(dchain ch, time_t time)
  requires true;
  ensures ch == expire_n_indexes(ch, time, 0);
  {
    switch(ch) { case dchain(alist, size, l, h):
      take_0(get_expired_indexes_fp(time, alist));
    }
  }
  @*/


/* @
  lemma void erase_nothing<K1,K2,V>(dmap<K1,K2,V> m)
  requires true;
  ensures dmap_erase_all_fp(m, nil) == m;
  {
    switch(m) { case dmap(ks1, ks2, idxs):
    }
  }
  @*/

int expire_items/*@<K1,K2,V> @*/(struct DoubleChain* chain,
                                 struct DoubleMap* map,
                                 time_t time)
/*@ requires dmappingp<K1,K2,V>(?m, ?kp1, ?kp2, ?hsh1, ?hsh2,
                                ?fvp, ?bvp, ?rof, ?vsz,
                                ?vk1, ?vk2, ?rp1, ?rp2, map) &*&
             double_chainp(?ch, chain) &*&
             dchain_index_range_fp(ch) < INT_MAX &*&
             dmap_dchain_coherent<K1,K2,V>(m, ch); @*/
/*@ ensures dmappingp<K1,K2,V>(?nm,
                               kp1, kp2, hsh1, hsh2, fvp, bvp, rof, vsz,
                               vk1, vk2, rp1, rp2, map) &*&
            nm == dmap_erase_all_fp
                               (m, dchain_get_expired_indexes_fp(ch, time),
                                vk1, vk2) &*&
            double_chainp(?nch, chain) &*&
            nch == dchain_expire_old_indexes_fp(ch, time) &*&
            dmap_dchain_coherent<K1,K2,V>(nm, nch) &*&
            result == length(dchain_get_expired_indexes_fp(ch, time)); @*/
{

  int count = 0;
  int index = -1;
  //@ expire_0_indexes(ch, time);
  //@ assert take(count, dchain_get_expired_indexes_fp(ch, time)) == nil;
  //@ double_chainp_to_sorted(ch);
  //@ dchain_expired_indexes_limited(ch, time);
  //@ double_chain_nodups(ch);
  while (dchain_expire_one_index(chain, &index, time))
    /*@ invariant double_chainp(expire_n_indexes(ch, time, count), chain) &*&
                  dchain_is_sortedp(ch) &*&
                  dmappingp(dmap_erase_all_fp
                              (m, take(count, dchain_get_expired_indexes_fp
                                               (ch, time)),
                               vk1, vk2),
                            kp1, kp2, hsh1, hsh2, fvp, bvp, rof, vsz,
                            vk1, vk2, rp1, rp2, map) &*&
                  dmap_dchain_coherent(dmap_erase_all_fp
                                        (m, take(count,
                                                 dchain_get_expired_indexes_fp
                                                  (ch, time)),
                                         vk1, vk2),
                                        expire_n_indexes(ch, time, count)) &*&
                  integer(&index, _) &*&
                  dchain_nodups(expire_n_indexes(ch, time, count)) &*&
                  0 <= count &*&
                  count <= length(dchain_get_expired_indexes_fp(ch, time)); @*/
  {
    /*@ dmap<K1,K2,V> cur_m = dmap_erase_all_fp
                               (m, take(count, dchain_get_expired_indexes_fp
                                                (ch, time)),
                                vk1, vk2);
                                @*/
    //@ dchain cur_ch = expire_n_indexes(ch, time, count);
    //@ coherent_old_index_used(cur_m, cur_ch);
    //@ coherent_same_cap(cur_m, cur_ch);
    dmap_erase(map, index);
    //@ expire_n_plus_one(ch, time, count);
    //@ dchain_still_more_to_expire(ch, time, count);
    ++count;
    /*@ dmap_erase_another_one(m, take(count-1, dchain_get_expired_indexes_fp(ch, time)),
                               dchain_get_oldest_index_fp(cur_ch),
                               vk1, vk2);
                               @*/
    //@ dchain_oldest_allocated(cur_ch);
    //@ coherent_expire_one(cur_m, cur_ch, dchain_get_oldest_index_fp(cur_ch), vk1, vk2);
  }
  /*@ if(dchain_is_empty_fp(expire_n_indexes(ch, time, count))) {
      expired_all(ch, time, count);
    } else {
      no_more_expired(ch, time, count);
    }
    @*/
  //@ assert count == length(dchain_get_expired_indexes_fp(ch, time));
  //@ assert take(count, dchain_get_expired_indexes_fp(ch, time)) == dchain_get_expired_indexes_fp(ch, time);

#ifdef DUMP_PERF_VARS
  NF_PERF_DEBUG("Expire_items:Success:Expired flows:%d",count);
#endif
  
  return count;
  //@ destroy_dchain_is_sortedp(ch);
  //@ destroy_dchain_nodups(expire_n_indexes(ch, time, count));
}

/*@
  lemma void vector_erase_all_same_len<t>(list<pair<t, bool> > vec, list<int> indices)
  requires true;
  ensures length(vec) == length(vector_erase_all_fp(vec, indices));
  {
    assume(false);//TODO
  }
  @*/

int expire_items_single_map/*@ <kt> @*/(struct DoubleChain* chain,
                                        struct Vector* vector,
                                        struct Map* map,
                                        time_t time)
/*@ requires mapp<kt>(map, ?kp, ?hsh, ?recp, mapc(?cap, ?m, ?addrs)) &*&
             vectorp<kt>(vector, kp, ?v, ?vaddrs) &*&
             true == forall2(v, vaddrs, (kkeeper)(addrs)) &*&
             double_chainp(?ch, chain) &*&
             length(v) == cap &*&
             dchain_index_range_fp(ch) < INT_MAX &*&
             map_vec_chain_coherent<kt>(m, v, ch); @*/
/*@ ensures mapp<kt>(map, kp, hsh, recp, mapc(cap, ?nm, ?naddrs)) &*&
            vectorp<kt>(vector, kp, ?nv, vaddrs) &*&
            double_chainp(?nch, chain) &*&
            nch == dchain_expire_old_indexes_fp(ch, time) &*&
            nm == map_erase_all_fp(m, vector_get_values_fp(v, dchain_get_expired_indexes_fp(ch, time))) &*&
            naddrs == map_erase_all_fp(addrs, vector_get_values_fp(v, dchain_get_expired_indexes_fp(ch, time))) &*&
            nv == vector_erase_all_fp(v, dchain_get_expired_indexes_fp(ch, time)) &*&
            map_vec_chain_coherent<kt>(nm, nv, nch) &*&
            length(nv) == length(v) &*&
            result == length(dchain_get_expired_indexes_fp(ch, time)); @*/
{
  int count = 0;
  int index = -1;
  //@ expire_0_indexes(ch, time);
  //@ assert take(count, dchain_get_expired_indexes_fp(ch, time)) == nil;
  //@ double_chainp_to_sorted(ch);
  //@ dchain_expired_indexes_limited(ch, time);
  //@ double_chain_nodups(ch);
  //@ vector_addrs_same_len_nodups(vector);
  //@ dchain cur_ch = ch;
  //@ list<pair<kt, uint32_t> > cur_m = m;
  //@ list<pair<kt, void*> > cur_addrs = addrs;
  //@ list<pair<kt, bool> > cur_v = v;
  while (dchain_expire_one_index(chain, &index, time))
    /*@ invariant double_chainp(cur_ch, chain) &*&
                  dchain_is_sortedp(ch) &*&
                  cur_ch == expire_n_indexes(ch, time, count) &*&
                  mapp<kt>(map, kp, hsh, recp,
                                    mapc(cap, cur_m, cur_addrs)) &*&
                  cur_m == map_erase_all_fp
                             (m, vector_get_values_fp
                                   (v, take(count,
                                            dchain_get_expired_indexes_fp
                                              (ch, time)))) &*&
                  cur_addrs == map_erase_all_fp
                                 (addrs, vector_get_values_fp
                                           (v, take(count,
                                                    dchain_get_expired_indexes_fp
                                                      (ch, time)))) &*&
                  vectorp<kt>(vector, kp, cur_v, vaddrs) &*&
                  true == forall2(cur_v, vaddrs, (kkeeper)(cur_addrs)) &*&
                  length(vaddrs) == length(cur_v) &*&
                  cur_v == vector_erase_all_fp(v, take(count,
                                                       dchain_get_expired_indexes_fp
                                                         (ch, time))) &*&
                  map_vec_chain_coherent<kt>(cur_m, cur_v, cur_ch) &*&
                  integer(&index, _) &*&
                  dchain_nodups(cur_ch) &*&
                  0 <= count &*&
                  count <= length(dchain_get_expired_indexes_fp(ch, time));
      @*/
  {
    /*@ mvc_coherent_bounds(cur_m, cur_v, cur_ch);
      @*/
    //@ dchain_oldest_allocated(cur_ch);
    //@ mvc_coherent_index_busy(cur_m, cur_v, cur_ch, dchain_get_oldest_index_fp(cur_ch));
    void* key;
    vector_borrow_half(vector, index, &key);
    //@ assert *&key |-> ?key_pointer;
    //@ assert [_]kp(key_pointer, ?k);
    //@ forall2_nth(cur_v, vaddrs, (kkeeper)(cur_addrs), index);
    map_erase(map, key, &key);
    vector_return_full(vector, index, key);
    //@ dchain_still_more_to_expire(ch, time, count);
    //@ expire_n_plus_one(ch, time, count);
    ++count;
    //@ list<int> cur_exp_indices = take(count-1, dchain_get_expired_indexes_fp(ch, time));
    //@ map_erase_another_one(m, vector_get_values_fp(v, cur_exp_indices), k);
    //@ map_erase_another_one(addrs, vector_get_values_fp(v, cur_exp_indices), k);
    //@ vector_erase_one_more(v, cur_exp_indices, index);

    //@ dchain_remove_keeps_nodups(cur_ch, index);
    //@ vector_erase_all_same_len(v, cur_exp_indices);
    //@ vector_erase_all_keeps_val(v, cur_exp_indices, index);
    //@ kkeeper_erase_one(vaddrs, cur_v, cur_addrs, index);
    //@ mvc_coherent_expire_one(cur_m, cur_v, cur_ch, index, k);
    //@ cur_ch = dchain_remove_index_fp(cur_ch, index);
    //@ cur_m = map_erase_fp(cur_m, k);
    //@ cur_addrs = map_erase_fp(cur_addrs, k);
    //@ cur_v = vector_erase_fp(cur_v, index);

    //@ vector_get_values_append(v, cur_exp_indices, index, k);
  }
  /*@ if(dchain_is_empty_fp(expire_n_indexes(ch, time, count))) {
    expired_all(ch, time, count);
    } else {
    no_more_expired(ch, time, count);
    }
    @*/
  //@ vector_erase_all_same_len(v, take(count, dchain_get_expired_indexes_fp(ch, time)));

#ifdef DUMP_PERF_VARS
  NF_PERF_DEBUG("Expire_items:Success:Expired flows:%d",count);
#endif
  return count;
  //@ destroy_dchain_is_sortedp(ch);
  //@ destroy_dchain_nodups(cur_ch);
}

int expire_items_single_map2/*@ <kt> @*/(struct DoubleChain* chain,
                                         struct Vector* vector,
                                         struct Map* map,
                                         time_t time)
{
  int count = 0;
  int index = -1;
  while (dchain_expire_one_index(chain, &index, time))
  {
    void* key;
    vector_borrow_half(vector, index, &key);
    map_erase(map, key, &key);
    vector_return_full(vector, index, key);
    ++count;
  }

#ifdef DUMP_PERF_VARS
  NF_PERF_DEBUG("Expire_items:Success:Expired flows:%d",count);
#endif
  return count;
}
