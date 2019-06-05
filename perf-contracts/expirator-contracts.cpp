#include "expirator-contracts.h"

/* Perf contracts */

long expire_items_contract_0(std::string metric, std::vector<long> values)
{
  long n_expired = values[2];
  long recent = values[3];
  long constant, constant_dependency, dynamic, dynamic_dependency;
  dynamic = 0;
  dynamic_dependency = 0;
  if (metric == "instruction count")
  {
    constant = 30;
    dynamic = 10 * n_expired;
  }
  else if (metric == "memory instructions")
  {
    constant = 13;
    dynamic = 1 * n_expired;
  }
  else if (metric == "execution cycles")
  {
    constant = 0 * DRAM_LATENCY + 13 * L1_LATENCY + 21;
    dynamic = (1 * L1_LATENCY + 11) * n_expired;
  }
  constant_dependency =
      dchain_expire_one_index_contract(metric, 0); // Eventually fail to expire
  long num_collisions = values[1];

  if (n_expired > 0)
  {
    /* Hack - switch the order of recent_flow and expired_items*/
    values[2] = recent;
    values[3] = n_expired;
    // Bounding the unique flows
    values[1] = 0;
    long unique_flow_hashes = (n_expired - num_collisions - 1) > 0
                                  ? (n_expired - num_collisions - 1)
                                  : 0;
    dynamic_dependency += dchain_expire_one_index_contract(metric, 1);
    if (recent)
    {
      dynamic_dependency += dmap_erase_contract_1(metric, values);
    }
    else
    {
      dynamic_dependency += dmap_erase_contract_0(metric, values);
    }

    dynamic_dependency *= unique_flow_hashes;

    values[1] = num_collisions;
    int penalised_flows =
        n_expired >= (num_collisions + 1) ? num_collisions + 1 : n_expired;
    for (int i = 0; i < penalised_flows; i++)
    {
      dynamic_dependency += dchain_expire_one_index_contract(metric, 1);
      if (recent)
      {
        dynamic_dependency += dmap_erase_contract_1(metric, values);
      }
      else
      {
        dynamic_dependency += dmap_erase_contract_0(metric, values);
      }
      values[0]--;
      values[1]--;

      for (std::vector<long>::iterator iter = values.begin();
           iter != values.end(); ++iter)
      {
        if (*iter < 0)
        {
          *iter = 0;
        }
      }
    }
  }
  return (constant + constant_dependency + dynamic + dynamic_dependency);
}

long expire_items_single_map_contract_0(std::string metric,
                                        std::vector<long> values)
{
  long n_expired = values[2];
  long constant, constant_dependency, dynamic, dynamic_dependency;
  dynamic_dependency = 0;
  if (metric == "instruction count")
  {
    constant = 33;
    dynamic = 19 * n_expired;
  }
  else if (metric == "memory instructions")
  {
    constant = 15;
    dynamic = 4 * n_expired;
  }
  else if (metric == "execution cycles")
  {

    constant = 0 * DRAM_LATENCY + 15 * L1_LATENCY +
               22; // Have not gone through patterns here. In progress
    dynamic = (4 * L1_LATENCY + 21) * n_expired;
  }
  constant_dependency =
      dchain_expire_one_index_contract(metric, 0); // Eventually fail to expire
  long num_collisions = values[1];

  if (n_expired > 0)
  {
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
    for (int i = 0; i < penalised_flows; i++)
    {
      dynamic_dependency += dchain_expire_one_index_contract(metric, 1);
      dynamic_dependency += map_erase_contract_0(metric, values);
      dynamic_dependency += vector_borrow_contract_0(metric, values);
      dynamic_dependency += vector_return_contract_0(metric, values);
      values[0]--;
      values[1]--;

      for (std::vector<long>::iterator iter = values.begin();
           iter != values.end(); ++iter)
      {
        if (*iter < 0)
        {
          *iter = 0;
        }
      }
    }
  }
  return (constant + constant_dependency + dynamic + dynamic_dependency);
}

/* Cstate contracts */

std::map<std::string, std::set<int>>
expire_items_cstate_contract_0(std::vector<long> values)
{

  //long n_expired = values[4];

  std::map<std::string, std::set<int>> cstate;
  cstate["rsp"] = {-8, -16, -24, -32, -40, -48, -60, -72, -80};
  std::map<std::string, int> dependency_calls;
  dependency_calls["rsp"] = -80;
  cstate = add_cstate_dependency(
      cstate, dependency_calls,
      dchain_expire_one_index_cstate_contract(0)); /*Final fail*/
  return cstate;
}

std::map<std::string, std::set<int>>
expire_items_single_map_cstate_contract_0(std::vector<long> values)
{

  //long n_expired = values[2];

  std::map<std::string, std::set<int>> cstate;
  cstate["rsp"] = {-8, -16, -24, -32, -40, -48, -56, -64};
  std::map<std::string, int> dependency_calls;
  dependency_calls["rsp"] = -64;
  cstate = add_cstate_dependency(
      cstate, dependency_calls,
      dchain_expire_one_index_cstate_contract(0)); /*Final fail*/
  return cstate;
}
