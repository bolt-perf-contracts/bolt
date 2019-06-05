#include "klee/perf-contracts.h"

/* Subcontract files */
#include "contract-params.h"
#include "dpdk-contracts.h"
#include "helper-contracts.h"
#include "map-impl-contracts.h"
#include "dchain-contracts.h"
#include "vector-contracts.h"
#include "ip-opt-contracts.h"
#include "lpm-contracts.h"
#include "map-contracts.h"
#include "dmap-contracts.h"
#include "expirator-contracts.h"
#include "cht-contracts.h"

typedef long (*perf_calc_fn_ptr)(std::string, std::vector<long>);
typedef std::map<std::string, std::set<int>> (*cstate_fn_ptr)(
    std::vector<long>);

std::vector<std::string> supported_metrics;
std::vector<std::string> fn_names;
std::map<std::string, std::string> user_variables;
std::map<std::string, std::set<std::string>> optimization_variables;
std::map<std::string, std::map<int, std::string>> constraints;
std::map<std::string, std::map<int, perf_calc_fn_ptr>> perf_fn_ptrs;
std::map<std::string, std::map<int, cstate_fn_ptr>> cstate_fn_ptrs;

std::map<std::string, std::vector<std::string>> fn_user_variables;
std::map<std::string, std::vector<std::string>> fn_optimization_variables;
std::map<std::string, std::vector<std::string>> fn_variables;
std::vector<std::string> relevant_vars;
std::vector<long> relevant_vals;

/* Contract stitcher for concrete state */
std::map<std::string, std::set<int>>
add_cstate_dependency(std::map<std::string, std::set<int>> caller,
                      std::map<std::string, int> caller_state,
                      std::map<std::string, std::set<int>> callee)
{

  std::map<std::string, std::set<int>> temp;
  for (auto it : caller_state)
  {
    if (callee.find(it.first) != callee.end())
    {
      for (auto it1 : callee[it.first])
      {
        temp[it.first].insert(it1 + it.second);
      }
    }
  }

  for (auto it : caller)
  {
    std::set<int> tempset;
    std::set_union(std::begin(it.second), std::end(it.second),
                   std::begin(temp[it.first]), std::end(temp[it.first]),
                   std::inserter(tempset, std::begin(tempset)));
    caller[it.first] = tempset;
  }

  return caller;
}

/* Functions called by workflow  */

bool check_metric(std::string metric)
{
  if (std::find(supported_metrics.begin(), supported_metrics.end(), metric) !=
      supported_metrics.end())
  {
    return true;
  }
  return false;
}
void contract_init()
{
  supported_metrics = {"execution cycles", "instruction count",
                       "memory instructions"};
  fn_names = {
      "dchain_allocate", "dchain_allocate_new_index",
      "dchain_rejuvenate_index",
      "map_allocate", "map_get",
      "map_put",
      "map_erase", "dmap_allocate",
      "dmap_get_a", "dmap_get_b",
      "dmap_put", "dmap_erase",
      "dmap_get_value", "expire_items",
      "expire_items_single_map",
      "vector_allocate", "vector_borrow_full",
      "vector_borrow_half", "vector_return_full",
      "vector_return_half", "handle_packet_timestamp",
      "lpm_init", "lpm_lookup",
      "trace_reset_buffers", "lb_find_preferred_available_backend",
      "flood", /*Hack*/
  };
  /* List of variables the user can set */
  user_variables = {
      {"dmap_occupancy",
       "(Sub w32 (ReadLSB w32 0 initial_dmap_capacity) (w32 1))"},
      {"map_occupancy",
       "(Sub w32 (ReadLSB w32 0 initial_map_capacity) (w32 1))"},
      {"Num_bucket_traversals", "(ReadLSB w32 0 initial_dmap_occupancy)"},
      {"Num_hash_collisions", "(ReadLSB w32 0 initial_Num_bucket_traversals)"},
      {"expired_flows", "(ReadLSB w32 0 initial_map_occupancy)"},
      {"timestamp_option", {"(w32 1)"}},
      {"lpm_stages", "(ReadLSB w32 0 initial_lpm_stages)"},
      {"available_backends", "(ReadLSB w32 0 initial_map_capacity)"},
      {"recent_flow", "(ReadLSB w32 0 initial_recent_flow)"},
  };

  /* List of shadow variables that allow more precise contracts. 
  The user cannot specify these unless they are also UVs*/

  optimization_variables = {
      {"expired_flows",
       {"(w32 0)", "(ReadLSB w32 0 initial_dmap_occupancy)"}},
      {"dchain_out_of_space", {"(w32 0)", "(w32 1)"}},
      {"dmap_has_this_key", {"(w32 0)", "(w32 1)"}},
      {"map_has_this_key", {"(w32 0)", "(w32 1)"}},
      {"timestamp_option", {"(w32 0)", "(w32 1)"}},
      {"lpm_stages", {"(w32 1)", "(w32 2)"}},
      {"recent_flow", {"(w32 0)", "(w32 1) "}},
  };

  /* Map of function name to user variables */
  fn_user_variables = {
      {"map_get", {"Num_bucket_traversals", "Num_hash_collisions"}},
      {"map_put", {"Num_bucket_traversals"}},
      {"map_erase", {"Num_bucket_traversals", "Num_hash_collisions"}},
      {"dmap_get_a", {"Num_bucket_traversals", "Num_hash_collisions"}},
      {"dmap_get_b", {"Num_bucket_traversals", "Num_hash_collisions"}},
      {"dmap_put", {"Num_bucket_traversals"}},
      {"dmap_erase", {"Num_bucket_traversals", "Num_hash_collisions"}},
      {"expire_items", {"Num_bucket_traversals", "Num_hash_collisions", "expired_flows"}},
      {"expire_items_single_map", {"Num_bucket_traversals", "Num_hash_collisions", "expired_flows"}},
      {"handle_packet_timestamp", {"timestamp_option"}},
      {"lpm_lookup", {"lpm_stages"}},
      {"lb_find_preferred_available_backend", {"available_backends"}},
  };

  /* Map of function name to shadow variable. If a variable is both a UV and a shadow variable
  it must be listed in the fn_user_variables map. */
  fn_optimization_variables = {
      {"dchain_allocate_new_index", {"dchain_out_of_space"}},
      {"map_get", {"map_has_this_key"}},
      {"dmap_get_a", {"dmap_has_this_key", "recent_flow"}},
      {"dmap_get_b", {"dmap_has_this_key", "recent_flow"}},
      {"dmap_put", {"recent_flow"}},
      {"dmap_erase", {"recent_flow"}},
      {"dmap_get_value", {"recent_flow"}},
      {"expire_items", {"recent_flow"}},
  };

  fn_variables.insert(fn_user_variables.begin(), fn_user_variables.end());
  for (std::map<std::string, std::vector<std::string>>::iterator i =
           fn_optimization_variables.begin();
       i != fn_optimization_variables.end(); ++i)
  {
    if (fn_variables.find(i->first) == fn_variables.end())
    {
      fn_variables[i->first] = i->second;
    }
    else
    {
      fn_variables[i->first].insert(fn_variables[i->first].end(), i->second.begin(), i->second.end());
    }
  }
  constraints = {
      //{"fn_name",{ {0,{"constraint1","constraint2" }} , {1,{"constraint1",
      //"constraint2"}}  }},
      {"dchain_allocate", {{0, "true"}}},
      {"dchain_allocate_new_index",
       {{0, "(Eq 0 (ReadLSB w32 0 current_dchain_out_of_space))"},
        {1,
         "(Eq false (Eq 0 (ReadLSB w32 0 current_dchain_out_of_space)))"}}},
      {"dchain_rejuvenate_index", {{0, "true"}}},
      {"map_allocate", {{0, "true"}}},
      {"map_get",
       {{0, "(Eq false (Eq 0 (ReadLSB w32 0 current_map_has_this_key)))"},
        {1, "(Eq 0 (ReadLSB w32 0 current_map_has_this_key))"}}},
      {"map_put", {{0, "true"}}},
      {"map_erase", {{0, "true"}}},
      {"dmap_allocate", {{0, "true"}}},
      {"dmap_get_a", {{0, "(And (Eq false (Eq 0 (ReadLSB w32 0 current_dmap_has_this_key))) (Eq 0 (ReadLSB w32 0 current_recent_flow)))"}, {1, "(And (Eq false (Eq 0 (ReadLSB w32 0 current_dmap_has_this_key)))  (Eq false (Eq 0 (ReadLSB w32 0 current_recent_flow))))"}, {2, "(Eq 0 (ReadLSB w32 0 current_dmap_has_this_key))"}}},
      {"dmap_get_b", {{0, "(And (Eq false (Eq 0 (ReadLSB w32 0 current_dmap_has_this_key))) (Eq 0 (ReadLSB w32 0 current_recent_flow)))"}, {1, "(And (Eq false (Eq 0 (ReadLSB w32 0 current_dmap_has_this_key))) (Eq false (Eq 0 (ReadLSB w32 0 current_recent_flow))))"}, {2, "(Eq 0 (ReadLSB w32 0 current_dmap_has_this_key))"}}},
      {"dmap_put", {{0, "(Eq 0 (ReadLSB w32 0 current_recent_flow))"}, {1, "(Eq false (Eq 0 (ReadLSB w32 0 current_recent_flow)))"}}},
      {"dmap_get_value", {{0, "(Eq 0 (ReadLSB w32 0 current_recent_flow))"}, {1, "(Eq false (Eq 0 (ReadLSB w32 0 current_recent_flow)))"}}},
      {"dmap_erase", {{0, "(Eq 0 (ReadLSB w32 0 current_recent_flow))"}, {1, "(Eq false (Eq 0 (ReadLSB w32 0 current_recent_flow)))"}}},
      {"expire_items", {{0, "true"}}},
      {"expire_items_single_map", {{0, "true"}}},
      {"vector_allocate", {{0, "true"}}},
      {"vector_borrow_full", {{0, "true"}}},
      {"vector_borrow_half", {{0, "true"}}},
      {"vector_return_full", {{0, "true"}}},
      {"vector_return_half", {{0, "true"}}},
      {"handle_packet_timestamp", {{0, "(Eq 1 (ReadLSB w32 0 current_timestamp_option))"}}},
      {"lpm_init", {{0, "true"}}},
      {"lpm_lookup", {{0, "true"}}},
      {"trace_reset_buffers", {{0, "true"}}},
      {"lb_find_preferred_available_backend", {{0, "true"}}},
      {"flood", {{0, "true"}}},
  };

  perf_fn_ptrs = {
      {"dchain_allocate", {{0, &dchain_allocate_contract_0}}},
      {"dchain_allocate_new_index",
       {{0, &dchain_allocate_new_index_contract_0},
        {1, &dchain_allocate_new_index_contract_1}}},
      {"dchain_rejuvenate_index",
       {{0, &dchain_rejuvenate_index_contract_0}}},
      {"map_allocate", {{0, &map_allocate_contract_0}}},
      {"map_get", {{0, &map_get_contract_0}, {1, &map_get_contract_1}}},
      {"map_put", {{0, &map_put_contract_0}}},
      {"map_erase", {{0, &map_erase_contract_0}}},
      {"dmap_allocate", {{0, &dmap_allocate_contract_0}}},
      {"dmap_get_a",
       {{0, &dmap_get_a_contract_0}, {1, &dmap_get_a_contract_1}, {2, &dmap_get_a_contract_2}}},
      {"dmap_get_b",
       {{0, &dmap_get_b_contract_0}, {1, &dmap_get_b_contract_1}, {2, &dmap_get_b_contract_2}}},
      {"dmap_put", {{0, &dmap_put_contract_0}, {1, &dmap_put_contract_1}}},
      {"dmap_erase", {{0, &dmap_erase_contract_0}, {1, &dmap_erase_contract_1}}},
      {"dmap_get_value", {{0, &dmap_get_value_contract_0}, {1, &dmap_get_value_contract_1}}},
      {"expire_items", {{0, &expire_items_contract_0}}},
      {"expire_items_single_map",
       {{0, &expire_items_single_map_contract_0}}},
      {"vector_allocate", {{0, &vector_allocate_contract_0}}},
      {"vector_borrow_half", {{0, &vector_borrow_contract_0}}},
      {"vector_borrow_full", {{0, &vector_borrow_contract_0}}},
      {"vector_return_half", {{0, &vector_return_contract_0}}},
      {"vector_return_full", {{0, &vector_return_contract_0}}},
      {"lpm_init", {{0, &lpm_init_contract_0}}},
      {"lpm_lookup", {{0, &lpm_lookup_contract_0}}},
      {"trace_reset_buffers", {{0, &trace_reset_buffers_contract_0}}},
      {"handle_packet_timestamp",
       {{0, &handle_packet_timestamp_contract_0}}},
      {"lb_find_preferred_available_backend",
       {{0, &lb_find_preferred_available_backend_contract_0}}},
      {"flood", {{0, &flood_contract_0}}},
  };

  cstate_fn_ptrs = {
      {"dchain_allocate", {{0, &dchain_allocate_cstate_contract_0}}},
      {"dchain_allocate_new_index",
       {{0, &dchain_allocate_new_index_cstate_contract_0},
        {1, &dchain_allocate_new_index_cstate_contract_1}}},
      {"dchain_rejuvenate_index",
       {{0, &dchain_rejuvenate_index_cstate_contract_0}}},
      {"map_allocate", {{0, &map_allocate_cstate_contract_0}}},
      {"map_get",
       {{0, &map_get_cstate_contract_0}, {1, &map_get_cstate_contract_1}}},
      {"map_put", {{0, &map_put_cstate_contract_0}}},
      {"map_erase", {{0, &map_erase_cstate_contract_0}}},
      {"dmap_allocate", {{0, &dmap_allocate_cstate_contract_0}}},
      {"dmap_get_a", {{0, &dmap_get_a_cstate_contract_0}, {1, &dmap_get_a_cstate_contract_0}, {2, &dmap_get_a_cstate_contract_1}}},
      {"dmap_get_b", {{0, &dmap_get_b_cstate_contract_0}, {1, &dmap_get_b_cstate_contract_0}, {2, &dmap_get_b_cstate_contract_1}}},
      {"dmap_put", {{0, &dmap_put_cstate_contract_0}, {1, &dmap_put_cstate_contract_0}}},
      {"dmap_erase", {{0, &dmap_erase_cstate_contract_0}, {1, &dmap_erase_cstate_contract_0}}},
      {"dmap_get_value", {{0, &dmap_get_value_cstate_contract_0}, {1, &dmap_get_value_cstate_contract_0}}},
      {"expire_items", {{0, &expire_items_cstate_contract_0}}},
      {"expire_items_single_map",
       {{0, &expire_items_single_map_cstate_contract_0}}},
      {"vector_allocate", {{0, &vector_allocate_cstate_contract_0}}},
      {"vector_borrow_half", {{0, &vector_borrow_cstate_contract_0}}},
      {"vector_borrow_full", {{0, &vector_borrow_cstate_contract_0}}},
      {"vector_return_half", {{0, &vector_return_cstate_contract_0}}},
      {"vector_return_full", {{0, &vector_return_cstate_contract_0}}},
      {"lpm_init", {{0, &lpm_init_cstate_contract_0}}},
      {"lpm_lookup", {{0, &lpm_lookup_cstate_contract_0}}},
      {"trace_reset_buffers",
       {{0, &trace_reset_buffers_cstate_contract_0}}},
      {"handle_packet_timestamp",
       {{0, &handle_packet_timestamp_cstate_contract_0}}},
      {"lb_find_preferred_available_backend",
       {{0, &lb_find_preferred_available_backend_cstate_contract_0}}},
      {"flood", {{0, &flood_cstate_contract_0}}},
  };

  std::cerr << "Loading Performance Contracts." << std::endl;
}
/* **************************************** */
std::map<std::string, std::string> contract_get_user_variables()
{
  return user_variables;
}
/* **************************************** */
std::set<std::string> contract_get_contracts()
{
  return std::set<std::string>(fn_names.begin(), fn_names.end());
}
/* **************************************** */
std::set<std::string> contract_get_metrics()
{
  return std::set<std::string>(supported_metrics.begin(),
                               supported_metrics.end());
}
/* **************************************** */
bool contract_has_contract(std::string function_name)
{
  if (std::find(fn_names.begin(), fn_names.end(), function_name) !=
      fn_names.end())
  {
    return true;
  }
  return false;
}
/* **************************************** */
std::map<std::string, std::set<std::string>>
contract_get_optimization_variables()
{
  return optimization_variables;
}
/* **************************************** */
std::set<std::string> contract_get_contract_symbols(std::string function_name)
{
  return std::set<std::string>(fn_variables[function_name].begin(),
                               fn_variables[function_name].end());
}
/* **************************************** */
std::set<std::string> contract_get_symbols()
{
  return {

      /* Map symbols */
      "array map_capacity[4] : w32 -> w8 = symbolic",
      "array current_map_capacity[4] : w32 -> w8 = symbolic",
      "array initial_map_capacity[4] : w32 -> w8 = symbolic",
      "array map_occupancy[4] : w32 -> w8 = symbolic",
      "array current_map_occupancy[4] : w32 -> w8 = symbolic",
      "array initial_map_occupancy[4] : w32 -> w8 = symbolic",
      "array Num_bucket_traversals[4] : w32 -> w8 = symbolic",
      "array current_Num_bucket_traversals[4] : w32 -> w8 = symbolic",
      "array initial_Num_bucket_traversals[4] : w32 -> w8 = symbolic",
      "array Num_hash_collisions[4] : w32 -> w8 = symbolic",
      "array current_Num_hash_collisions[4] : w32 -> w8 = symbolic",
      "array initial_Num_hash_collisions[4] : w32 -> w8 = symbolic",
      "array map_has_this_key[4] : w32 -> w8 = symbolic",
      "array current_map_has_this_key[4] : w32 -> w8 = symbolic",
      "array initial_map_has_this_key[4] : w32 -> w8 = symbolic",

      /* Double Map symbols */
      "array dmap_capacity[4] : w32 -> w8 = symbolic",
      "array current_dmap_capacity[4] : w32 -> w8 = symbolic",
      "array initial_dmap_capacity[4] : w32 -> w8 = symbolic",
      "array dmap_occupancy[4] : w32 -> w8 = symbolic",
      "array current_dmap_occupancy[4] : w32 -> w8 = symbolic",
      "array initial_dmap_occupancy[4] : w32 -> w8 = symbolic",
      "array dmap_has_this_key[4] : w32 -> w8 = symbolic",
      "array current_dmap_has_this_key[4] : w32 -> w8 = symbolic",
      "array initial_dmap_has_this_key[4] : w32 -> w8 = symbolic",

      /* Double Chain symbols */
      "array dchain_out_of_space[4] : w32 -> w8 = symbolic",
      "array current_dchain_out_of_space[4] : w32 -> w8 = symbolic",
      "array initial_dchain_out_of_space[4] : w32 -> w8 = symbolic",

      /* Expirator symbols */
      "array expired_flows[4] : w32 -> w8 = symbolic",
      "array current_expired_flows[4] : w32 -> w8 = symbolic",
      "array initial_expired_flows[4] : w32 -> w8 = symbolic",

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
      "array recent_flow[4] : w32 -> w8 = symbolic",
      "array current_recent_flow[4] : w32 -> w8 = symbolic",
      "array initial_recent_flow[4] : w32 -> w8 = symbolic",
  };
}
/* **************************************** */
int contract_num_sub_contracts(std::string function_name)
{
  return constraints[function_name].size();
}
/* **************************************** */
std::string contract_get_subcontract_constraints(std::string function_name,
                                                 int sub_contract_idx)
{
  return constraints[function_name][sub_contract_idx];
}
/* **************************************** */
long contract_get_sub_contract_performance(std::string function_name,
                                           int sub_contract_idx, std::string metric,
                                           std::map<std::string, long> variables)
{
  if (!(check_metric(metric)))
    return -1;

  relevant_vars = fn_variables[function_name];
  relevant_vals.clear();
  for (std::vector<std::string>::iterator i = relevant_vars.begin();
       i != relevant_vars.end(); ++i)
  {
    if (variables.find(*i) == variables.end())
    {
      std::cerr << "Required variable " << *i << " not sent for function "
                << function_name << std::endl;
      return -1;
    }
    else
      relevant_vals.insert(relevant_vals.end(), variables[*i]);
  }
  perf_calc_fn_ptr fn_ptr;
  fn_ptr = perf_fn_ptrs[function_name][sub_contract_idx];
  assert(fn_ptr);
  long perf = fn_ptr(metric, relevant_vals);
  if (metric == "memory instructions" && perf > 0)
  {
    perf = perf - 1;
  }
  return perf;
}

/* **************************************** */
std::map<std::string, std::set<int>>
contract_get_concrete_state(std::string function_name, int sub_contract_idx,
                            std::map<std::string, long> variables)
{

  relevant_vars = fn_variables[function_name];
  relevant_vals.clear();
  for (std::vector<std::string>::iterator i = relevant_vars.begin();
       i != relevant_vars.end(); ++i)
  {
    if (variables.find(*i) == variables.end())
    {
      std::cerr << "Required variable " << *i << " not sent for function "
                << function_name << std::endl;
      assert(0);
    }
    else
      relevant_vals.insert(relevant_vals.end(), variables[*i]);
  }

  cstate_fn_ptr fn_ptr;
  fn_ptr = cstate_fn_ptrs[function_name][sub_contract_idx];
  assert(fn_ptr && "Function pointer not found for cstate call\n");

  std::map<std::string, std::set<int>> cstate = fn_ptr(relevant_vals);
  return cstate;
}

int main() {}