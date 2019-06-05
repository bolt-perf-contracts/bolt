/* Contracts for Helper Functions */

#include "helper-contracts.h"

/*  Perf contracts */

long ether_addr_hash_contract(std::string metric)
{
  long constant;
  if (metric == "instruction count")
    constant = 3;
  else if (metric == "memory instructions")
    constant = 3;
  else if (metric == "execution cycles")
    constant = 0 * DRAM_LATENCY + 3 * L1_LATENCY + 0;
  return constant;
}

long ether_addr_eq_contract(std::string metric, long success)
{
  long constant;
  if (metric == "instruction count")
    constant = 31;
  else if (metric == "memory instructions")
    constant = 9;
  else if (metric == "execution cycles")
    constant = 2 * DRAM_LATENCY + 7 * L1_LATENCY + 0;
  return constant;
}
long int_key_eq_contract(std::string metric, long success)
{
  long constant;
  if (metric == "instruction count")
    constant = 20;
  else if (metric == "memory instructions")
    constant = 12;
  else if (metric == "execution cycles")
    constant = 1 * DRAM_LATENCY + 11 * L1_LATENCY + 17;
  return constant;
}

long ext_key_eq_contract(std::string metric, long success)
{
  long constant;
  if (metric == "instruction count")
    constant = 20;
  else if (metric == "memory instructions")
    constant = 12;
  else if (metric == "execution cycles")
    constant = 1 * DRAM_LATENCY + 11 * L1_LATENCY + 17;
  return constant;
}

long int_key_hash_contract(std::string metric)
{
  long constant;
  if (metric == "instruction count")
    constant = 40;
  else if (metric == "memory instructions")
    constant = 8;
  else if (metric == "execution cycles")
    constant = 0 * DRAM_LATENCY + 8 * L1_LATENCY + 55;
  return constant;
}

long ext_key_hash_contract(std::string metric)
{
  long constant;
  if (metric == "instruction count")
    constant = 40;
  else if (metric == "memory instructions")
    constant = 8;
  else if (metric == "execution cycles")
    constant = 0 * DRAM_LATENCY + 8 * L1_LATENCY + 55;
  return constant;
}

long flow_cpy_contract(std::string metric, long recent)
{
  long constant;
  if (metric == "instruction count")
    constant = 43;
  else if (metric == "memory instructions")
    constant = 44;
  else if (metric == "execution cycles")
    constant = 42 * L1_LATENCY + 1;
  if (recent)
    constant += 2 * L1_LATENCY;
  else
    constant += 2 * DRAM_LATENCY;
  return constant;
}

long flow_destroy_contract(std::string metric)
{
  long constant;
  if (metric == "instruction count")
    constant = 1;
  else if (metric == "memory instructions")
    constant = 2;
  else if (metric == "execution cycles")
    constant = 0 * DRAM_LATENCY + 2 * L1_LATENCY + 1;
  return constant;
}

long flow_extract_keys_contract(std::string metric)
{
  long constant;
  if (metric == "instruction count")
    constant = 4;
  else if (metric == "memory instructions")
    constant = 4;
  else if (metric == "execution cycles")
    constant = 0 * DRAM_LATENCY + 4 * L1_LATENCY + 2;
  return constant;
}

long flow_pack_keys_contract(std::string metric)
{
  long constant;
  if (metric == "instruction count")
    constant = 1;
  else if (metric == "memory instructions")
    constant = 2;
  else if (metric == "execution cycles")
    constant = 0 * DRAM_LATENCY + 2 * L1_LATENCY + 1;
  return constant;
}

/* Concrete state contracts */

std::map<std::string, std::set<int>> ether_addr_hash_cstate_contract()
{

  std::map<std::string, std::set<int>> cstate;
  return cstate;
}

std::map<std::string, std::set<int>> ether_addr_eq_cstate_contract()
{

  std::map<std::string, std::set<int>> cstate;
  return cstate;
}

std::map<std::string, std::set<int>> int_key_hash_cstate_contract()
{

  std::map<std::string, std::set<int>> cstate;
  return cstate;
}

std::map<std::string, std::set<int>> ext_key_hash_cstate_contract()
{

  std::map<std::string, std::set<int>> cstate;
  return cstate;
}

std::map<std::string, std::set<int>> int_key_eq_cstate_contract()
{

  std::map<std::string, std::set<int>> cstate;
  return cstate;
}

std::map<std::string, std::set<int>> ext_key_eq_cstate_contract()
{

  std::map<std::string, std::set<int>> cstate;
  return cstate;
}

std::map<std::string, std::set<int>> flow_cpy_cstate_contract()
{

  std::map<std::string, std::set<int>> cstate;
  return cstate;
}

std::map<std::string, std::set<int>> flow_destroy_cstate_contract()
{

  std::map<std::string, std::set<int>> cstate;
  return cstate;
}

std::map<std::string, std::set<int>> flow_extract_keys_cstate_contract()
{

  std::map<std::string, std::set<int>> cstate;
  return cstate;
}

std::map<std::string, std::set<int>> flow_pack_keys_cstate_contract()
{

  std::map<std::string, std::set<int>> cstate;
  return cstate;
}
