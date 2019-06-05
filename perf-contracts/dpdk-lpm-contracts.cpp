#include "lpm-contracts.h"

/* Perf contracts */

long lpm_init_contract_0(std::string metric, std::vector<long> values)
{
  return 0;
}
long lpm_lookup_contract_0(std::string metric, std::vector<long> values)
{
  long lpm_stages = values[0];
  long constant;
  if (metric == "instruction count")
  {
    constant = (lpm_stages == 2) ? 19 : 13;
  }
  else if (metric == "memory instructions")
  {
    constant = (lpm_stages == 2) ? 4 : 2;
  }
  else if (metric == "execution cycles")
  {
    constant = (lpm_stages == 2) ? 3 * DRAM_LATENCY + 1 * L1_LATENCY + 15
                                 : 1 * DRAM_LATENCY + 1 * L1_LATENCY + 11;
  }
  return constant;
}

/* Cstate contracts */

std::map<std::string, std::set<int>>
lpm_init_cstate_contract_0(std::vector<long> values)
{

  std::map<std::string, std::set<int>> cstate;
  return cstate;
}
std::map<std::string, std::set<int>>
lpm_lookup_cstate_contract_0(std::vector<long> values)
{

  std::map<std::string, std::set<int>> cstate;
  return cstate;
}