#include "dchain-contracts.h"

/* Perf contracts */
long dchain_is_index_allocated_contract(std::string metric)
{
  long constant;
  if (metric == "instruction count")
  {
    constant = 13;
  }
  else if (metric == "memory instructions")
  {
    constant = 4;
  }
  else if (metric == "execution cycles")
  {
    constant = (1) * DRAM_LATENCY + 3 * L1_LATENCY +
               9; // Have not gone through patterns here. In progress
  }
  return constant;
}

long dchain_expire_one_index_contract(std::string metric, long success)
{
  long constant;
  if (metric == "instruction count")
  {
    if (success)
    {
      constant = 50;
    }
    else
    {
      constant = 30;
    }
  }
  else if (metric == "memory instructions")
  {
    if (success)
    {
      constant = 25;
    }
    else
    {
      constant = 15;
    }
  }
  else if (metric == "execution cycles")
  {
    if (success)
    {
      constant = (2) * DRAM_LATENCY + 23 * L1_LATENCY + 28; // Have not gone
                                                            // through patterns
                                                            // here. In progress
                                                            // - Not of much use
    }
    else // We can reduce this to 9 if we condition on empty list
    {
      constant = (2) * DRAM_LATENCY + 13 * L1_LATENCY +
                 16; // Have not gone through patterns here. In progress
    }
  }
  return constant;
}

long dchain_allocate_contract_0(std::string metric, std::vector<long> values)
{
  return 0;
}
long dchain_allocate_contract_1(std::string metric, std::vector<long> values)
{
  return 0;
}
long dchain_allocate_new_index_contract_0(std::string metric,
                                          std::vector<long> values)
{
  long success = values[0];
  assert(success == 0); /* success is the inverse here*/
  long constant;
  if (metric == "instruction count")
  {
    constant = 35;
  }
  else if (metric == "memory instructions")
  {
    constant = 23;
  }
  else if (metric == "execution cycles")
  {
    constant = (3) * DRAM_LATENCY + 20 * L1_LATENCY + 22;
  }
  return constant;
}
long dchain_allocate_new_index_contract_1(std::string metric,
                                          std::vector<long> values)
{
  long success = values[0];
  assert(success); /* success is the inverse here*/
  long constant;
  if (metric == "instruction count")
  {
    constant = 19;
  }
  else if (metric == "memory instructions")
  {
    constant = 10;
  }
  else if (metric == "execution cycles")
  {
    constant = (3) * DRAM_LATENCY + 7 * L1_LATENCY +
               11; // Have not gone through patterns here. In progress
  }
  return constant;
}
long dchain_rejuvenate_index_contract_0(std::string metric,
                                        std::vector<long> values)
{
  long constant;
  if (metric == "instruction count")
  {
    constant = 34;
  }
  else if (metric == "memory instructions")
  {
    constant = 20;
  }
  else if (metric == "execution cycles")
  {
    constant = (3) * DRAM_LATENCY + 17 * L1_LATENCY + 18;
  }
  return constant;
}

/* Cstate contracts */

std::map<std::string, std::set<int>>
dchain_is_index_allocated_cstate_contract()
{

  std::map<std::string, std::set<int>> cstate;
  return cstate;
}
std::map<std::string, std::set<int>>
dchain_expire_one_index_cstate_contract(long success)
{

  std::map<std::string, std::set<int>> cstate;
  if (!success)
  {
    cstate["rsp"] = {-8, -16, -24, -32, -32};
  }
  return cstate;
}
std::map<std::string, std::set<int>>
dchain_allocate_cstate_contract_0(std::vector<long> values)
{

  std::map<std::string, std::set<int>> cstate;
  return cstate;
}
std::map<std::string, std::set<int>>
dchain_allocate_new_index_cstate_contract_0(std::vector<long> values)
{

  long success = values[0];
  assert(success == 0); /* success is the inverse here*/

  std::map<std::string, std::set<int>> cstate;
  cstate["rsp"] = {-8, -16, -24, -32, -40};
  return cstate;
}
std::map<std::string, std::set<int>>
dchain_allocate_new_index_cstate_contract_1(std::vector<long> values)
{

  long success = values[0];
  assert(success); /* success is the inverse here*/

  std::map<std::string, std::set<int>> cstate;
  return cstate;
}
std::map<std::string, std::set<int>>
dchain_rejuvenate_index_cstate_contract_0(std::vector<long> values)
{

  std::map<std::string, std::set<int>> cstate;
  cstate["rsp"] = {-8, -16, -24, -32};
  return cstate;
}
