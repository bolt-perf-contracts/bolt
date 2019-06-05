#include "vector-contracts.h"

/* Perf contracts*/

long vector_allocate_contract_0(std::string metric, std::vector<long> values)
{
  return 0;
}

long vector_borrow_contract_0(std::string metric, std::vector<long> values)
{
  long constant;
  if (metric == "instruction count")
  {
    constant = 5;
  }
  else if (metric == "memory instructions")
  {
    constant = 5;
  }
  else if (metric == "execution cycles")
  {
    constant = 1 * DRAM_LATENCY + 4 * L1_LATENCY + 2;
  }
  return constant;
}

long vector_return_contract_0(std::string metric, std::vector<long> values)
{
  long constant;
  if (metric == "instruction count")
  {
    constant = 1;
  }
  else if (metric == "memory instructions")
  {
    constant = 2;
  }
  else if (metric == "execution cycles")
  {
    constant = 1 + 2 * L1_LATENCY;
  }
  return constant;
}

/* Cstate contracts */

std::map<std::string, std::set<int>>
vector_allocate_cstate_contract_0(std::vector<long> values)
{

  std::map<std::string, std::set<int>> cstate;
  return cstate;
}

std::map<std::string, std::set<int>>
vector_borrow_cstate_contract_0(std::vector<long> values)
{

  std::map<std::string, std::set<int>> cstate;
  return cstate;
}

std::map<std::string, std::set<int>>
vector_return_cstate_contract_0(std::vector<long> values)
{

  std::map<std::string, std::set<int>> cstate;
  return cstate;
}