#ifndef LB_LOOP_H_INCLUDED
#define LB_LOOP_H_INCLUDED

#include "lib/containers/map.h"
#include "lib/containers/vector.h"
#include "lib/containers/double-chain.h"
#include "lib/coherence.h"
#include "lib/nf_time.h"

#include "lb_balancer.h"


/*@
predicate lb_loop_invariant(struct Map* indices, struct Vector* heap, struct Vector* backends, struct DoubleChain* chain,
                            time_t time, uint32_t flow_capacity) =
          mapp<lb_flowi>(indices, lb_flowp, lb_flow_hash_2, nop_true, mapc(flow_capacity, ?indicesi, ?indicesv)) &*&
          vectorp<lb_flowi>(heap, lb_flowp, ?heapv, ?heapa) &*&
          vectorp<lb_backendi>(backends, lb_backendp, ?backendsv, ?backendsa) &*&
          double_chainp(?chainv, chain) &*&
          true == forall2(heapv, heapa, (kkeeper)(indicesv)) &*&
          length(heapv) == flow_capacity &*&
          length(backendsv) == flow_capacity &*&
          map_vec_chain_coherent<lb_flowi>(indicesi, heapv, chainv) &*&
          true == forall(backendsv, snd) &*&
          last_time(time) &*&
          dchain_high_fp(chainv) <= time;
@*/

void lb_loop_iteration_assumptions(
				struct Map** flow_to_flow_id,
				struct Vector** flow_heap,
				struct DoubleChain** flow_chain,
				struct Vector** flow_id_to_backend_id,
				struct Vector** backend_ips,
				struct Vector** backends,
				struct Map** ip_to_backend_id,
				struct DoubleChain** active_backends,
				struct Vector** cht,
                                   time_t time, uint32_t backend_capacity, uint32_t flow_capacity);

void lb_loop_invariant_consume(
				struct Map** flow_to_flow_id,
				struct Vector** flow_heap,
				struct DoubleChain** flow_chain,
				struct Vector** flow_id_to_backend_id,
				struct Vector** backend_ips,
				struct Vector** backends,
				struct Map** ip_to_backend_id,
				struct DoubleChain** active_backends,
				struct Vector** cht,
                               time_t time, uint32_t backend_capacity, uint32_t flow_capacity);
/*@ requires *indices |-> ?indicesp &*& *heap |-> ?heapp &*& *backends |-> ?backendsp &*& *chain |-> ?chainp &*&
             lb_loop_invariant(indicesp, heapp, backendsp, chainp, time, flow_capacity); @*/
/*@ ensures *indices |-> indicesp &*& *heap |-> heapp &*& *backends |-> backendsp &*& *chain |-> chainp; @*/

void lb_loop_invariant_produce(
				struct Map** flow_to_flow_id,
				struct Vector** flow_heap,
				struct DoubleChain** flow_chain,
				struct Vector** flow_id_to_backend_id,
				struct Vector** backend_ips,
				struct Vector** backends,
				struct Map** ip_to_backend_id,
				struct DoubleChain** active_backends,
				struct Vector** cht,
                               time_t* time, uint32_t backend_capacity, uint32_t flow_capacity);
/*@ requires *indices |-> ?indicesp &*& *heap |-> ?heapp &*& *backends |-> ?backendsp &*& *chain |-> ?chainp &*&
             *time |-> _; @*/
/*@ ensures *indices |-> indicesp &*& *heap |-> heapp &*& *backends |-> backendsp &*& *chain |-> chainp &*&
            *time |-> ?t &*&
            lb_loop_invariant(indicesp, heapp, backendsp, chainp, t, flow_capacity); @*/

void lb_loop_iteration_begin(
				struct Map** flow_to_flow_id,
				struct Vector** flow_heap,
				struct DoubleChain** flow_chain,
				struct Vector** flow_id_to_backend_id,
				struct Vector** backend_ips,
				struct Vector** backends,
				struct Map** ip_to_backend_id,
				struct DoubleChain** active_backends,
				struct Vector** cht,
                             time_t time, uint32_t backend_capacity, uint32_t flow_capacity);

void lb_loop_iteration_end(
				struct Map** flow_to_flow_id,
				struct Vector** flow_heap,
				struct DoubleChain** flow_chain,
				struct Vector** flow_id_to_backend_id,
				struct Vector** backend_ips,
				struct Vector** backends,
				struct Map** ip_to_backend_id,
				struct DoubleChain** active_backends,
				struct Vector** cht,
                           time_t time, uint32_t backend_capacity, uint32_t flow_capacity);


#endif // LB_LOOP_H_INCLUDED
