/* Contract for DPDK Hack for freeing buffers */

#include "dpdk-contracts.h"

/* Perf contracts */

long trace_reset_buffers_contract_0(std::string metric,
                                    std::vector<long> values)
{
  long constant;
  if (metric == "instruction count")
    constant = 93;
  else if (metric == "memory instructions")
    constant = 15;
  else if (metric == "execution cycles")
    constant = 8 * DRAM_LATENCY + 7 * L1_LATENCY + 51;
  return constant;
}

long flood_contract_0(std::string metric, std::vector<long> values)
{
  long constant = 0;
  if (metric == "instruction count")
  {
    constant = 309;
  }
  else if (metric == "memory instructions")
  {
    constant = 177;
  }
  else if (metric == "execution cycles")
  {
    constant = 4 * DRAM_LATENCY + 173 * L1_LATENCY + 147;
  }
  return constant;
}

/* Cstate contracts */

std::map<std::string, std::set<int>>
trace_reset_buffers_cstate_contract_0(std::vector<long> values)
{

  /* TODO */
  std::map<std::string, std::set<int>> cstate;
  return cstate;
}

std::map<std::string, std::set<int>>
flood_cstate_contract_0(std::vector<long> values)
{

  std::map<std::string, std::set<int>> cstate;
  return cstate;
}